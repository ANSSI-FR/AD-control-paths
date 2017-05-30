/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"

/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("ControlMbx")
#define PLUGIN_KEYWORD      _T("CMB")
#define PLUGIN_DESCRIPTION  _T("Filters control ACE for Mailbox and Exchange DB objects");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
/*
cf. http://msdn.microsoft.com/en-us/library/windows/desktop/aa772285(v=vs.85).aspx
- ADS_RIGHT_DS_CREATE_CHILD
- ADS_RIGHT_DS_DELETE_CHILD
- ADS_RIGHT_DS_SELF
- ADS_RIGHT_DS_READ_PROP
- ADS_RIGHT_DS_WRITE_PROP
- ADS_RIGHT_DS_CONTROL_ACCESS
*/
#define OBJECT_TYPE_GUID_NEEDED(mask)  (mask & (ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_SELF|ADS_RIGHT_DS_READ_PROP|ADS_RIGHT_DS_WRITE_PROP|ADS_RIGHT_DS_CONTROL_ACCESS))

/*
Add-Mailboxpermission can set 6 different rights, which ends in the following mapping on the exchmailboxsd, except the last ones:

WD = ChangePermission = WRITE_DAC
WO = ChangeOwner = WRITE_OWNER
SD = DeleteItem = DELETE
LC = ExternalAccount = ADS_RIGHT_ACTRL_DS_LIST
CC = FullAccess = ADS_RIGHT_DS_CREATE_CHILD
RC = ReadPermission = READ_CONTROL

(SendAS/ReceiveAS are AD-permission = ADS_RIGHT_DS_CONTROL_ACCESS + Guid)

We work on the hypothesis of a logical mapping of generic rights.

WARNING: 
msExchMailboxSecurityDescriptor inherits from its containing DB, but ACE are not copied until modified.
So we need to pass both the mbxsd and the exch DB sd through this, even though we already passed the exch DB sd through ControlAD.
This is because Exchange-specific bits in the mask will be inherited on the mbx with the exchange interpretation.
So we write the DB control with the MSR writer and the mbx control with the MBR writer which targets email addresses.
*/

typedef enum _EXCH_RIGHTS_ENUM
{
	EXCH_RIGHT_DELETEITEM = 0x10000,
	EXCH_RIGHT_READPERMISSION = 0x20000,
	EXCH_RIGHT_CHANGEPERMISSION = 0x40000,
	EXCH_RIGHT_CHANGEOWNER = 0x80000,
	EXCH_RIGHT_GENERIC_READ = 0x80000000,
	EXCH_RIGHT_GENERIC_WRITE = 0x40000000,
	EXCH_RIGHT_GENERIC_EXECUTE = 0x20000000,
	EXCH_RIGHT_GENERIC_ALL = 0x10000000,
	EXCH_RIGHT_FULLACCESS = 0x1,
	EXCH_RIGHT_EXTERNALACCOUNT = 0x4,
} 	EXCH_RIGHTS_ENUM;



/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
	_In_ PLUGIN_API_TABLE const * const api
) {
	API_LOG(Bypass, _T("Keeps ACE allowing control relations on Mailbox and Exchange DB objects. Currently checked control relations are :"));
	API_LOG(Bypass, _T("- Generic rights : GENERIC_ALL, GENERIC_WRITE, GENERIC_READ"));
	API_LOG(Bypass, _T("- Standard rights : CHANGEPERMISSION, CHANGEOWNER, FULLACCESS"));
	API_LOG(Bypass, _T("This filter set 'relations' on the ACE, and must be combined with the MSR or MMR writers for these relations to be output."));
}

BOOL PLUGIN_FILTER_FILTERACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	DWORD dwAccessMask = 0;
	DWORD i = 0;

	//
	// Only "*_ALLOWED_*" ace types can allow control
	// But DENY ace on control parameters cannot be processed on a per-ace model in the control paths approach
	// So we dump them in 2 different files (see writer) and there's a post-processing for whole control paths after queries in the database.
	//

	//
	// Get properties
	//
	dwAccessMask = api->Ace.GetAccessMask(ace);

	//
	// Control case : Generic right GENERIC_READ
	//
	if (dwAccessMask & EXCH_RIGHT_GENERIC_READ)
		SET_RELATION(ace, GEN_RIGHT_READ);

	//
	// Control case : Generic right GENERIC_WRITE
	//
	if (dwAccessMask & EXCH_RIGHT_GENERIC_WRITE)
		SET_RELATION(ace, GEN_RIGHT_WRITE);

	//
	// Control case : Generic right GENERIC_ALL
	//
	if (dwAccessMask & EXCH_RIGHT_GENERIC_ALL)
		SET_RELATION(ace, GEN_RIGHT_ALL);

	//
	// Control case : Exchange ChangePermission
	//
	if (dwAccessMask & EXCH_RIGHT_CHANGEPERMISSION)
		SET_RELATION(ace, EXCH_CHANGEPERMISSION);

	//
	// Control case : Exchange ChangeOwner
	//
	if (dwAccessMask & EXCH_RIGHT_CHANGEOWNER)
		SET_RELATION(ace, EXCH_CHANGEOWNER);

	//
	// Control case : Exchange FullAccess
	//
	if (dwAccessMask & EXCH_RIGHT_FULLACCESS)
		SET_RELATION(ace, EXCH_FULLACCESS);

	//
	// Control case : Exchange ExternalAccount
	// TODO: additional research on this
	//
	/*
	if (dwAccessMask & EXCH_RIGHT_EXTERNALACCOUNT)
		SET_RELATION(ace, EXCH_EXTERNALACCOUNT);
	*/

	for (i = 0; i < ACE_REL_COUNT; i++) {
		if (HAS_RELATION(ace, i)) {
			return TRUE;
		}
	}
	return FALSE;
}