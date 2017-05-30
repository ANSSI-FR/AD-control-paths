/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"

/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("MapiFolder")
#define PLUGIN_KEYWORD      _T("MFO")
#define PLUGIN_DESCRIPTION  _T("Filters control ACE for Exchange MAPI Folders");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
/*
Set/Add-MailboxFolderpermission can set this, so can users through Outlook and other MAPI tools.
This is VERY peculiar, coming from an NT background.
https://blogs.technet.microsoft.com/exchange/2004/04/22/exchange-2000-acl-mechanism/
https://blogs.technet.microsoft.com/exchange/2004/06/23/exchange-2000-acl-conversion/

So we want: (we assume the dynamic SD of messages cannot be effected by indirect control such as write_dac)
On messages (OIIO): default access is O:PS.
fsdrightReadBody => 0x1

On folders (CI): default access is O:SY.
Check Owner => WRITE_DAC
fsdrightWriteSD => WRITE_DAC
fsdrightWriteOwner => WRITE_OWNER

Other are not mapped (generic etc.)



*/

typedef enum _MAPI_RIGHTS_ENUM
{
	MAPI_RIGHT_WRITESD = 0x40000,
	MAPI_RIGHT_WRITEOWNER = 0x80000,
	MAPI_RIGHT_READBODY = 0x1,
} 	MAPI_RIGHTS_ENUM;



/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
	_In_ PLUGIN_API_TABLE const * const api
) {
	API_LOG(Bypass, _T("Keeps ACE allowing control relations on Exchange folders and messages. Currently checked control relations are :"));
	API_LOG(Bypass, _T("- Folders: fsdrightWriteSD, fsdrightWriteOwner"));
	API_LOG(Bypass, _T("- Messages: fsdrightReadBody"));
	API_LOG(Bypass, _T("This filter set 'relations' on the ACE, and must be combined with the MSR or MMR writers for these relations to be output."));
}

BOOL PLUGIN_FILTER_FILTERACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	DWORD dwAccessMask = 0;
	DWORD dwAceFlags = 0;
	DWORD i = 0;

	//
	// Only "*_ALLOWED_*" ace types can allow control
	// But DENY ace on control parameters cannot be processed on a per-ace model in the control paths approach
	// HOWEVER the MAPI/Exchange 5 security descriptor model is different from NT and DENY ACE are in fact guards (xored bits of ALLOW).
	// So we will drop DENY here.
	//
	if (!IS_ALLOWED_ACE(ace->imported.raw)) {
		return FALSE;
	}

	//
	// Get properties
	//
	dwAccessMask = api->Ace.GetAccessMask(ace);
	dwAceFlags = ace->imported.raw->AceFlags;
	
	//
	// Control case : Folder fsdrightWriteSD
	//
	if ((dwAceFlags & CONTAINER_INHERIT_ACE) && (dwAccessMask & MAPI_RIGHT_WRITESD))
		SET_RELATION(ace, MAPI_WRITESD);

	//
	// Control case : Folder fsdrightWriteOwner
	//
	if ((dwAceFlags & CONTAINER_INHERIT_ACE) && (dwAccessMask & MAPI_RIGHT_WRITEOWNER))
		SET_RELATION(ace, MAPI_WRITEOWNER);

	//
	// Control case : Messages fsdrightReadBody
	//
	if ((dwAceFlags & OBJECT_INHERIT_ACE) && (dwAceFlags & INHERIT_ONLY_ACE) && (dwAccessMask & MAPI_RIGHT_READBODY))
		SET_RELATION(ace, MAPI_READBODY);

	

	for (i = 0; i < ACE_REL_COUNT; i++) {
		if (HAS_RELATION(ace, i)) {
			return TRUE;
		}
	}
	return FALSE;
}