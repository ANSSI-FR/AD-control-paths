/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"

/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("ControlAd")
#define PLUGIN_KEYWORD      _T("CAD")
#define PLUGIN_DESCRIPTION  _T("Filters control ACE for AD objects");

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

/* --- TYPES ---------------------------------------------------------------- */
typedef struct _CONTROL_GUID {
	GUID * guid;
	ACE_RELATION rel;
	LPTSTR *applicableClasses;
	DWORD applicableClassesCount;
} CONTROL_GUID, *PCONTROL_GUID;

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
// Guids used later
static GUID gcs_GuidExtendedRightUserForceChangePassword = { 0x00299570, 0x246d, 0x11d0, { 0xa7, 0x68, 0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29 } }; // _T("00299570-246d-11d0-a768-00aa006e0529");
static LPTSTR gcs_UserForceChangePasswordAppliesTo[3] = { _T("user"), _T("computer"), _T("inetorgperson") };

static GUID gcs_GuidExtendedRightDSReplicationGetChangesAll = { 0x1131f6ad, 0x9c07, 0x11d1, { 0xf7, 0x9f, 0x00, 0xc0, 0x4f, 0xc2, 0xdc, 0xd2 } }; // _T("1131f6ad-9c07-11d1-f79f-00c04fc2dcd2"); 
static LPTSTR gcs_DSReplicationGetChangesAllAppliesTo[1] = { _T("domaindns") };

static GUID gcs_GuidExtendedRightAdmPwd = { 0 }; // Runtime resolution required because this is generated at LAPS Schema extension
static LPTSTR gcs_AdmPwdAppliesTo[1] = { _T("computer") };
static BOOL isAdmPwdGuidResolved = FALSE;

static GUID gcs_GuidExtendedRightSendAs = { 0xab721a54, 0x1e2f, 0x11d0,{ 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b } }; // _T("ab721a54-1e2f-11d0-9819-00aa0040529b");
static LPTSTR gcs_SendAsAppliesTo[4] = { _T("user"), _T("computer"), _T("msds-managedserviceaccount"), _T("inetorgperson") };

static GUID gcs_GuidExtendedRightReceiveAs = { 0xab721a56, 0x1e2f, 0x11d0,{ 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b } }; // _T("ab721a56-1e2f-11d0-9819-00aa0040529b");
static LPTSTR gcs_ReceiveAsAppliesTo[4] = { _T("user"), _T("computer"), _T("msds-managedserviceaccount"), _T("inetorgperson") };

static GUID gcs_GuidPropertyMember = { 0xbf9679c0, 0x0de6, 0x11d0, { 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 } }; // _T("bf9679c0-0de6-11d0-a285-00aa003049e2");
static LPTSTR gcs_MemberAppliesTo[1] = { _T("group") };

static GUID gcs_GuidPropertyGpcFileSysPath = { 0xf30e3bc1, 0x9ff0, 0x11d1, { 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1} };
static LPTSTR gcs_GpcFileSysPathAppliesTo[1] = { _T("grouppolicycontainer") };

static GUID gcs_GuidPropertyGpLink = { 0xf30e3bbe, 0x9ff0, 0x11d1, { 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } }; // _T("f30e3bbe-9ff0-11d1-b603-0000f80367c1");
static LPTSTR gcs_GpLinkAppliesTo[3] = { _T("organizationalunit"),_T("domaindns"),_T("site") };

static GUID gcs_GuidPropertySPN = { 0xf3a64788, 0x5306, 0x11d1, { 0xa9, 0xc5, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } }; // _T("f3a64788-5306-11d1-a9c5-0000f80367c1");
static LPTSTR gcs_SPNAppliesTo[2] = { _T("user"), _T("inetorgperson") };

static GUID gcs_GuidPropertyAltSecIdentities = { 0x00fbf30c, 0x91fe, 0x11d1, { 0xae, 0xbc, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } }; // _T("00fbf30c-91fe-11d1-aebc-0000f80367c1");
static LPTSTR gcs_AltSecIdentitiesAppliesTo[4] = { _T("user"), _T("computer"), _T("msds-managedserviceaccount"), _T("inetorgperson") };

static GUID gcs_GuidPropertySetMembership = { 0xbc0ac240, 0x79a9, 0x11d0, { 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf } }; // _T("bc0ac240-79a9-11d0-9020-00c04fc2d4cf");
static LPTSTR gcs_MembershipAppliesTo[1] = { _T("group") };

static GUID gcs_GuidPropertySetPublicInfo = { 0xe48d0154, 0xbcf8, 0x11d1, { 0x87, 0x02, 0x00, 0xc0, 0x4f, 0xb9, 0x60, 0x50 } }; // _T("e48d0154-bcf8-11d1-8702-00c04fb96050");
static LPTSTR gcs_PublicInfoAppliesTo[4] = { _T("user"), _T("computer"), _T("msds-managedserviceaccount"), _T("inetorgperson") };

static GUID gcs_GuidValidatedWriteSelfMembership = { 0xbf9679c0, 0x0de6, 0x11d0, { 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 } }; // Same as gs_GuidPropertyMember. thanks AD. GUIDs are no longer "UNIQUE"
static LPTSTR gcs_SelfMembershipAppliesTo[1] = { _T("group") };


// Control properties and properties sets
static CONTROL_GUID gcs_GuidsControlProperties[] = {
        { &gcs_GuidPropertyAltSecIdentities, WRITE_PROP_ALTSECID, gcs_AltSecIdentitiesAppliesTo, ARRAY_COUNT(gcs_AltSecIdentitiesAppliesTo) },
        { &gcs_GuidPropertySPN, WRITE_PROP_SPN, gcs_SPNAppliesTo, ARRAY_COUNT(gcs_SPNAppliesTo) },
	{ &gcs_GuidPropertyMember, WRITE_PROP_MEMBER, gcs_MemberAppliesTo, ARRAY_COUNT(gcs_MemberAppliesTo) },
	{ &gcs_GuidPropertyGpLink, WRITE_PROP_GPLINK, gcs_GpLinkAppliesTo, ARRAY_COUNT(gcs_GpLinkAppliesTo)},
	{ &gcs_GuidPropertyGpcFileSysPath, WRITE_PROP_GPC_FILE_SYS_PATH, gcs_GpcFileSysPathAppliesTo, ARRAY_COUNT(gcs_GpcFileSysPathAppliesTo) },
	{ &gcs_GuidPropertySetMembership, WRITE_PROPSET_MEMBERSHIP, gcs_MembershipAppliesTo, ARRAY_COUNT(gcs_MembershipAppliesTo) },
	{ &gcs_GuidPropertySetPublicInfo, WRITE_PROPSET_PUBINFO, gcs_PublicInfoAppliesTo, ARRAY_COUNT(gcs_PublicInfoAppliesTo) },
};

// Control extended rights
static CONTROL_GUID gcs_GuidsControlExtendedRights[] = {
	{ &gcs_GuidExtendedRightUserForceChangePassword, EXT_RIGHT_FORCE_CHANGE_PWD, gcs_UserForceChangePasswordAppliesTo, ARRAY_COUNT(gcs_UserForceChangePasswordAppliesTo) },
	{ &gcs_GuidExtendedRightDSReplicationGetChangesAll, EXT_RIGHT_REPLICATION_GET_CHANGES_ALL, gcs_DSReplicationGetChangesAllAppliesTo, ARRAY_COUNT(gcs_DSReplicationGetChangesAllAppliesTo) },
	{ &gcs_GuidExtendedRightAdmPwd, EXT_RIGHT_ADM_PWD, gcs_AdmPwdAppliesTo, ARRAY_COUNT(gcs_AdmPwdAppliesTo) },
	{ &gcs_GuidExtendedRightSendAs, EXT_RIGHT_SEND_AS, gcs_SendAsAppliesTo, ARRAY_COUNT(gcs_SendAsAppliesTo) },
	{ &gcs_GuidExtendedRightReceiveAs, EXT_RIGHT_RECEIVE_AS, gcs_ReceiveAsAppliesTo, ARRAY_COUNT(gcs_ReceiveAsAppliesTo) },
};

// Control validated writes
static CONTROL_GUID gcs_GuidsControlValidatedWrites[] = {
	{ &gcs_GuidValidatedWriteSelfMembership, VAL_WRITE_SELF_MEMBERSHIP, gcs_SelfMembershipAppliesTo, ARRAY_COUNT(gcs_SelfMembershipAppliesTo) },
};


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
BOOL isObjectClassAdapted(
	_In_ PLUGIN_API_TABLE const * const api,
	_In_opt_ GUID *objectType,
	_In_ CONTROL_GUID controlGuid,
	_In_ LPTSTR *objectClasses,
	_In_ DWORD objectClassesCount
) {
	if (!objectType || GUID_EQ(objectType, controlGuid.guid))
		for (DWORD j = 0; j < controlGuid.applicableClassesCount; j++)
			if (api->Common.IsInSetOfStrings(controlGuid.applicableClasses[j], objectClasses, objectClassesCount, NULL))
				return TRUE;

	return FALSE;
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
	_In_ PLUGIN_API_TABLE const * const api
) {
	API_LOG(Bypass, _T("Keeps ACE allowing control relations on AD objects. Currently checked control relations are :"));
	API_LOG(Bypass, _T("- Generic rights : GENERIC_ALL, GENERIC_WRITE"));
	API_LOG(Bypass, _T("- Standard rights : WRITE_DAC, WRITE_OWNER"));
	API_LOG(Bypass, _T("- Extended rights : All, User-Force-Change-Password, DS-Replication-Get-Changes-All, AdmPwd (LAPS)"));
	API_LOG(Bypass, _T("- Validated writes : All, Self-Membership"));
	API_LOG(Bypass, _T("- Write properties : Member, GPLink, SPN, Alt-Security-Identities"));
	API_LOG(Bypass, _T("- Write property-sets : Membership, PublicInfo(SPN,alt-security-identities)"));
	API_LOG(Bypass, _T("This filter set 'relations' on the ACE, and must be combined with the MSR writer for these relations to be output."));
}

BOOL PLUGIN_FILTER_FILTERACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	/*
	Version 1:
	The reason we do not filter object classes here is to preserve the inherited control paths, even though some rights are "inadapted" to the current object.
	For example: reset pwd on OUs impacts only user children.
	We prefer those paths to go along the containers control, which makes for better visualization, and we see "original" ACEs (rst-pwd on OUs...).

	Version 2:
	Reality first, so inadapted rights are filtered and inherited ACEs are not. We get ~2*more vertices (admin->users rst-pwd...).

	Currently checked rights:

	- Generic rights :
		- GENERIC_WRITE
		- GENERIC_ALL

	- Standards rights :
		- WRITE_DAC
		- WRITE_OWNER

	- Object-specific rights :
		- WRITE_PROP :

	- Properties :
		- all (empty guid)
		- member => group class
		- gPLink (TODO : cannot find a property set containing this attr) => organizationalunit class
                - Alt-Security-Identities
                - SPN

	- Property sets :
		- membership (contains member) => group class (tested, even though MSDN and GUI seems to think groups cannot get this)
		- Public-Information (contains Service-Principal-Name => users/computers classes. Accounts can be kerberoasted etc. Contains Alt-Security-Identities => maps x509 certs to the user account)

	- Extended rights (ADS_RIGHT_DS_CONTROL_ACCESS) :
		- all (empty guid)
		- Reset password => user class
		- DS Replication Get Changes All => AD partitions (Domain-DNS, Configuration, DMD)
		- AdmPwd: this allows reading the LAPS password => computer class
		- SendAs: control on mailbox, to be written as smtp address
		- ReceiveAs: control on mailbox, to be written as smtp address

	- Validated writes (ADS_RIGHT_DS_SELF) :
		- all (empty guid)
		- Add/remove self as member => group class
	*/

	DWORD dwAccessMask = 0;
	DWORD dwFlags = 0;
	DWORD i = 0;
	PIMPORTED_OBJECT object = NULL;
	LPTSTR *objectClasses = NULL;
	DWORD objectClassesCount = 0;
	GUID *admPwdGuid = NULL;

	// We cannot do this in an initialize fn, as we need the schema cache
	if (!isAdmPwdGuidResolved) {
		admPwdGuid = api->Resolver.GetAdmPwdGuid();
		if (admPwdGuid) {
			RtlCopyMemory(&gcs_GuidExtendedRightAdmPwd, admPwdGuid, sizeof(GUID));
			API_LOG(Info, _T("ms-Mcs-AdmPwd found in schema: LAPS schema extension detected"));
		}
		else {
			API_LOG(Info, _T("ms-Mcs-AdmPwd cannot be found in schema: no LAPS schema extension detected"));
		}
		isAdmPwdGuidResolved = TRUE;
	}
	//
	// Only "*_ALLOWED_*" ace types can allow control
	// But DENY ace on control parameters cannot be processed on a per-ace model in the control paths approach
	// So we dump them in 2 different files (see writer) and there's a post-processing for whole control paths after queries in the database.
	//


	//
	// Get properties
	//
	dwAccessMask = api->Ace.GetAccessMask(ace);
	dwFlags = IS_OBJECT_ACE(ace->imported.raw) ? api->Ace.GetObjectFlags(ace) : 0;

	//
	// Control case : Generic right GENERIC_WRITE
	//
	if (dwAccessMask & ADS_RIGHT_GENERIC_WRITE)
		SET_RELATION(ace, GEN_RIGHT_WRITE);

	//
	// Control case : Generic right GENERIC_ALL
	//
	if (dwAccessMask & ADS_RIGHT_GENERIC_ALL)
		SET_RELATION(ace, GEN_RIGHT_ALL);

	//
	// Control case : Standard right WRITE_DAC
	//
	if (dwAccessMask & ADS_RIGHT_WRITE_DAC)
		SET_RELATION(ace, STAND_RIGHT_WRITE_DAC);

	//
	// Control case : Standard right WRITE_OWNER
	//
	if (dwAccessMask & ADS_RIGHT_WRITE_OWNER)
		SET_RELATION(ace, STAND_RIGHT_WRITE_OWNER);

	// Control cases where ObjectType GUID is needed
	if (OBJECT_TYPE_GUID_NEEDED(dwAccessMask)) {
		object = api->Resolver.ResolverGetAceObject(ace);
		if (!object) {
			LOG(Dbg, _T("ace object %s cannot be found in cache"), ace->imported.objectDn);
			return FALSE;
		}
		objectClasses = object->imported.objectClassesNames;
		objectClassesCount = object->computed.objectClassCount;

		if (!(dwFlags & ACE_OBJECT_TYPE_PRESENT)) {

			// extended right + empty guid == all extended rights
			if (dwAccessMask & ADS_RIGHT_DS_CONTROL_ACCESS)
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlExtendedRights); i++)
					if (isObjectClassAdapted(api, NULL, gcs_GuidsControlExtendedRights[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, EXT_RIGHT_ALL);

			// validated write + empty guid == all validated rights
			if (dwAccessMask & ADS_RIGHT_DS_SELF)
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlValidatedWrites); i++)
					if (isObjectClassAdapted(api, NULL, gcs_GuidsControlValidatedWrites[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, VAL_WRITE_ALL);

			// write prop + empty guid == write all properties
			if (dwAccessMask & ADS_RIGHT_DS_WRITE_PROP)
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlProperties); i++)
					if (isObjectClassAdapted(api, NULL, gcs_GuidsControlProperties[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, WRITE_PROP_ALL);
			
		}
		else {
			GUID * objectType = api->Ace.GetObjectTypeAce(ace);
			// Control cases with extended rights
			if (dwAccessMask & ADS_RIGHT_DS_CONTROL_ACCESS) {
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlExtendedRights); i++)
					if (isObjectClassAdapted(api, objectType, gcs_GuidsControlExtendedRights[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, gcs_GuidsControlExtendedRights[i].rel);
			}
			// Control cases with validated writes
			if (dwAccessMask & ADS_RIGHT_DS_SELF) {
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlValidatedWrites); i++)
					if (isObjectClassAdapted(api, objectType, gcs_GuidsControlValidatedWrites[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, gcs_GuidsControlValidatedWrites[i].rel);
			}

			// Control cases with write properties and write property sets
			if (dwAccessMask & ADS_RIGHT_DS_WRITE_PROP) {
				for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlProperties); i++)
					if (isObjectClassAdapted(api, objectType, gcs_GuidsControlProperties[i], objectClasses, objectClassesCount))
						SET_RELATION(ace, gcs_GuidsControlProperties[i].rel);
			}
		}
	} // OBJECT_TYPE_GUID_NEEDED(dwAccessMask))

	for (i = 0; i < ACE_REL_COUNT; i++) {
		if (HAS_RELATION(ace, i)) {
			return TRUE;
		}
	}
	return FALSE;
}
