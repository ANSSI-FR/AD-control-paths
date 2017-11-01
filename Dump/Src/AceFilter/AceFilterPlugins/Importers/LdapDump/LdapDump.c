/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("LdapDump")
#define PLUGIN_KEYWORD      _T("LDP")
#define PLUGIN_DESCRIPTION  _T("Imports ACE, OBJ and SCH from LdapDump's oufiles (ace/obj/sch.ldpdmp.tsv)");
#define LDAPDUMP_HEAP_NAME  _T("LDPHEAP")

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FINALIZE;
PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_GETNEXTACE;
PLUGIN_DECLARE_GETNEXTOBJ;
PLUGIN_DECLARE_GETNEXTSCH;
PLUGIN_DECLARE_RESETREADING;
PLUGIN_DECLARE_FREEACE;
PLUGIN_DECLARE_FREEOBJECT;
PLUGIN_DECLARE_FREESCHEMA;



/* --- DEFINES -------------------------------------------------------------- */
#define LDPDUMP_ACE_TOKEN_COUNT         (2)
#define LDPDUMP_OBJ_TOKEN_COUNT        (22)
#define LDPDUMP_SCH_TOKEN_COUNT      (5)
#define LDPDUMP_HEADER_FIRST_TOKEN      _T("dn")


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _ACE_TSV_TOKENS {
	LdpAceDn = 0,
	LdpAceNtSecurityDescriptor = 1,
} ACE_TSV_TOKENS;

typedef enum _SCH_TSV_TOKENS {
	LdpSchDn = 0,
	LdpSchSchemaIDGUID = 1,
	LdpSchGovernsID = 2,
	LdpSchDefaultSecurityDescriptor = 3,
	LdpSchLDAPDisplayName = 4,
} SCH_TSV_TOKENS;

typedef enum _OBJ_TSV_TOKENS {
	LdpListDn = 0,
	LdpListObjectClass = 1,
	LdpListObjectSid = 2,
	LdpListAdminCount = 3,
	LdpListMember = 4,
	LdpListGPLink = 5,
	LdpListPrimaryGroupID = 6,
	LdpListSIDHistory = 7,
	LdpListCN = 8,
	LdpListManagedBy,
	LdpListRevealOnDemand,
	LdpListNeverReveal,
	LdpListMail,
	LdpListHomeMDB,
	LdpListMSExchRoleEntries,
	LdpListMSExchUserLink,
	LdpListMSExchRoleLink,
	LdpListUAC,
	LdpListAllowedToDelegateTo,
	LdpListAllowedToActOnBehalf,
	LdpListSPN,
	LdpListDnsHostName,
} OBJ_TSV_TOKENS;


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PUTILS_HEAP gs_hHeapLdapDump = INVALID_HANDLE_VALUE;
static LPTSTR gs_AceFileName = NULL;
static LPTSTR gs_ObjFileName = NULL;
static LPTSTR gs_SchFileName = NULL;

static CSV_HANDLE gs_AceFile = 0;
static CSV_HANDLE gs_ObjFile = 0;
static CSV_HANDLE gs_SchFile = 0;

static DWORD gs_AceFileHeaderCount = 0;
static DWORD gs_ObjFileHeaderCount = 0;
static DWORD gs_SchFileHeaderCount = 0;

static LONGLONG gs_AcePosition = 0;
static LONGLONG gs_ObjPosition = 0;
static LONGLONG gs_SchPosition = 0;

static PVOID gs_AceMapping = NULL;
static PVOID gs_ObjMapping = NULL;
static PVOID gs_SchMapping = NULL;

static LPTSTR *gs_AceTokens = { 0 };
static PSECURITY_DESCRIPTOR gs_CurrentSd = NULL;
static PACL gs_CurrentDacl = NULL;
static DWORD gs_CurrentAceIndex = 0;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void ResetAceVars(
	_In_ PLUGIN_API_TABLE const * const api
) {
	if (gs_CurrentSd) {
		ApiHeapFreeX(gs_hHeapLdapDump, gs_CurrentSd);
	}
	gs_CurrentSd = NULL;
	gs_CurrentDacl = NULL;
	gs_CurrentAceIndex = 0;
	if (gs_AceTokens && gs_AceTokens[0]) {
		api->InputCsv.CsvRecordArrayHeapFree(gs_AceTokens, gs_AceFileHeaderCount);
	}
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
	_In_ PLUGIN_API_TABLE const * const api
) {
	API_LOG(Bypass, _T("Specify the infiles with the <ldpace>, <ldpobj> and <ldpsch> plugin options (no default)"));
	API_LOG(Bypass, _T("This importers can be used to import ACE/SCH/OBJ separately (but at lease one must be specified)"));
}

BOOL PLUGIN_GENERIC_INITIALIZE(
	_In_ PLUGIN_API_TABLE const * const api
) {
	BOOL bResult = FALSE;

	bResult = ApiHeapCreateX(&gs_hHeapLdapDump, LDAPDUMP_HEAP_NAME, NULL);
	if (API_FAILED(bResult)) {
		return ERROR_VALUE;
	}

	gs_AceFileName = api->Common.GetPluginOption(_T("ldpace"), FALSE);
	gs_ObjFileName = api->Common.GetPluginOption(_T("ldpobj"), FALSE);
	gs_SchFileName = api->Common.GetPluginOption(_T("ldpsch"), FALSE);

	if (!gs_AceFileName && !gs_ObjFileName && !gs_SchFileName) {
		API_FATAL(_T("You must specify at least one plugin option between 'ldpace', 'ldpobj' and 'ldpsch'"));
	}
	API_LOG(Info, _T("Infile ace is <%s>"), gs_AceFileName);
	API_LOG(Info, _T("Infile obj is <%s>"), gs_ObjFileName);
	API_LOG(Info, _T("Infile sch is <%s>"), gs_SchFileName);

	if (gs_AceFileName) {
		bResult = api->InputCsv.CsvOpenRead(gs_AceFileName, &gs_AceFileHeaderCount, NULL, &gs_AceFile);
		if (!bResult) {
			API_FATAL(_T("Failed to open input file <%s>"), gs_AceFileName);
		}
		else if (gs_AceFileHeaderCount != LDPDUMP_ACE_TOKEN_COUNT) {
			API_FATAL(_T("Wrong number of fields in ACE file for <%s>"), gs_AceFileName);
		}
		else {
			API_LOG(Dbg, _T("Successfully opened file <%s>"), gs_AceFileName);
		}
	}

	if (gs_ObjFileName) {
		bResult = api->InputCsv.CsvOpenRead(gs_ObjFileName, &gs_ObjFileHeaderCount, NULL, &gs_ObjFile);
		if (!bResult) {
			API_FATAL(_T("Failed to open input file <%s>"), gs_ObjFileName);
		}
		else if (gs_ObjFileHeaderCount != LDPDUMP_OBJ_TOKEN_COUNT) {
			API_FATAL(_T("Wrong number of fields in OBJ file for <%s>"), gs_ObjFileName);
		}
		else {
			API_LOG(Dbg, _T("Successfully opened file <%s>"), gs_ObjFileName);
		}
	}

	if (gs_SchFileName) {
		bResult = api->InputCsv.CsvOpenRead(gs_SchFileName, &gs_SchFileHeaderCount, NULL, &gs_SchFile);
		if (!bResult) {
			API_FATAL(_T("Failed to open input file <%s>"), gs_SchFileName);
		}
		else if (gs_SchFileHeaderCount != LDPDUMP_SCH_TOKEN_COUNT) {
			API_FATAL(_T("Wrong number of fields in SCH file for <%s>"), gs_SchFileName);
		}
		else {
			API_LOG(Dbg, _T("Successfully opened file <%s>"), gs_SchFileName);
		}
	}
	ResetAceVars(api);
	return TRUE;
}


BOOL PLUGIN_GENERIC_FINALIZE(
	_In_ PLUGIN_API_TABLE const * const api
) {
	if (gs_AceFileName) api->InputCsv.CsvClose(&gs_AceFile);
	if (gs_ObjFileName) api->InputCsv.CsvClose(&gs_ObjFile);
	if (gs_SchFileName) api->InputCsv.CsvClose(&gs_SchFile);
	if (!ApiHeapDestroyX(&gs_hHeapLdapDump)) {
		API_LOG(Err, _T("Failed to close destroy heap: <%u>"), GetLastError());
		return FALSE;
	}
	return TRUE;
}


void PLUGIN_IMPORTER_RESETREADING(
	_In_ PLUGIN_API_TABLE const * const api,
	_In_ IMPORTER_DATATYPE type
) {
	UNREFERENCED_PARAMETER(api);

	switch (type) {
	case ImporterAce:
		if (!gs_AceFileName) {
			API_FATAL(_T("Plugin option <ldpace> not specified, but requesting ACE operation (read-reset)"));
		}

		api->InputCsv.CsvResetFile(gs_AceFile);
		ResetAceVars(api);
		break;

	case ImporterObject:
		if (!gs_ObjFileName) {
			API_FATAL(_T("Plugin option <ldpobj> not specified, but requesting OBJ operation (read-reset)"));
		}

		api->InputCsv.CsvResetFile(gs_ObjFile);
		break;

	case ImporterSchema:
		if (!gs_SchFileName) {
			API_FATAL(_T("Plugin option <ldpsch> not specified, but requesting SCH operation (read-reset)"));
		}

		api->InputCsv.CsvResetFile(gs_SchFile);
		break;

	default:
		API_FATAL(_T("Unsupported read-reset operation <%u>"), type);
	}
}

BOOL PLUGIN_IMPORTER_GETNEXTACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	BOOL bResult = FALSE;
	PACE_HEADER currentAce = NULL;
	size_t len = 0;
	BOOL isSDPresent = FALSE;

	if (!gs_AceFileName) {
		API_FATAL(_T("reading ACE while 'ldpace' option has not been specified"));
	}

	while (!gs_CurrentDacl || gs_CurrentAceIndex >= gs_CurrentDacl->AceCount) {
		// get the next DACL
		while (!isSDPresent) {
			API_LOG(Dbg, _T("Getting next DACL..."));
			ResetAceVars(api);
			bResult = api->InputCsv.CsvGetNextRecord(gs_AceFile, &gs_AceTokens, NULL);
			if (!bResult) {
				// No more DACL => No more ACE to import
				return FALSE;
			}
			if (!STR_EMPTY(gs_AceTokens[LdpAceDn]))
				CharLower(gs_AceTokens[LdpAceDn]);
			if (!STR_EMPTY(gs_AceTokens[LdpAceNtSecurityDescriptor])) {
				isSDPresent = TRUE;
			}
		}

		BOOL bDaclPresent = FALSE, bDaclDefaulted = FALSE;
		len = STRLEN_IFNOTNULL(gs_AceTokens[LdpAceNtSecurityDescriptor]);

		gs_CurrentSd = ApiHeapAllocX(gs_hHeapLdapDump, (DWORD)(len / 2));
		api->Common.Unhexify(gs_CurrentSd, gs_AceTokens[LdpAceNtSecurityDescriptor]);
		bResult = IsValidSecurityDescriptor(gs_CurrentSd);
		if (!bResult) {
			API_FATAL(_T("Invalid SD for <%s>"), gs_AceTokens[LdpAceDn]);
		}

		bResult = GetSecurityDescriptorDacl(gs_CurrentSd, &bDaclPresent, &gs_CurrentDacl, &bDaclDefaulted);
		if (!bResult || !bDaclPresent) {
			API_FATAL(_T("Cannot get DACL for <%s> : <%u>"), gs_AceTokens[LdpAceDn], GetLastError());
		}
		else if (!gs_CurrentDacl) {
			API_LOG(Dbg, _T("NULL DACL for <%s>, object is UNSECURED, returning Full Access for Everyone"), gs_AceTokens[LdpAceDn]);
			ace->imported.source = AceFromActiveDirectory;
			ace->imported.raw = ApiHeapAllocX(gs_hHeapLdapDump, 20);
			api->Common.Unhexify((PBYTE)(ace->imported.raw), _T("00021400FF010F00010100000000000100000000"));
			ace->imported.objectDn = ApiStrDupX(gs_hHeapLdapDump, gs_AceTokens[LdpAceDn]);
			if (!IsValidSid(api->Ace.GetTrustee(ace))) {
				API_FATAL(_T("ace does not have a valid trustee sid <%s>"), ace->imported.objectDn);
			}
			gs_CurrentAceIndex++;
			return TRUE;
		}
		else if (gs_CurrentDacl->AceCount == 0) {
			API_LOG(Dbg, _T("Empty DACL for <%s>, no access except by owner"), gs_AceTokens[LdpAceDn]);
			isSDPresent = FALSE;
			continue;
		}
		else {
			API_LOG(Dbg, _T("Getting ACE <%u/%u> for <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_AceTokens[LdpAceDn]);
		}
	}
	// Get the next ACE in the current DACL and inc the index
	bResult = GetAce(gs_CurrentDacl, gs_CurrentAceIndex, &currentAce);
	if (!bResult) {
		API_FATAL(_T("Cannot get ACE <%u/%u> of <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_AceTokens[LdpAceDn]);
	}
	// Import the ACE
	ace->imported.source = AceFromActiveDirectory;
	ace->imported.raw = ApiHeapAllocX(gs_hHeapLdapDump, currentAce->AceSize);
	memcpy(ace->imported.raw, currentAce, currentAce->AceSize);
	ace->imported.objectDn = ApiStrDupX(gs_hHeapLdapDump, gs_AceTokens[LdpAceDn]);
	if (!IsValidSid(api->Ace.GetTrustee(ace))) {
		API_FATAL(_T("ace does not have a valid trustee sid <%s>"), ace->imported.objectDn);
	}
	gs_CurrentAceIndex++;
	return TRUE;
}


BOOL PLUGIN_IMPORTER_GETNEXTOBJ(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_OBJECT obj
) {
	BOOL bResult = FALSE;
	LPTSTR *tokens = { 0 };
	DWORD i = 0;
	LPTSTR objectClassList = NULL;
	LPTSTR next = NULL;

	if (!gs_ObjFileName) {
		API_FATAL(_T("reading OBJ while 'ldpobj' option has not been specified"));
	}

	bResult = api->InputCsv.CsvGetNextRecord(gs_ObjFile, &tokens, NULL);
	if (!bResult || !tokens) {
		// No more objects to import
		return FALSE;
	}
	// Lower case of every records, because of inconsistencies, i.e. gplinks
	for (i = 0; i < LDPDUMP_OBJ_TOKEN_COUNT; i++) {
		if (!STR_EMPTY(tokens[i]))
			CharLower(tokens[i]);
	}
	obj->imported.dn = ApiStrDupX(gs_hHeapLdapDump, tokens[LdpListDn]);
	obj->imported.adminCount = STR_EMPTY(tokens[LdpListAdminCount]) ? 0 : 1;

	if (!STR_EMPTY(tokens[LdpListMail])) {
		obj->imported.mail = ApiStrDupX(gs_hHeapLdapDump, tokens[LdpListMail]);
	}

	if (!STR_EMPTY(tokens[LdpListObjectClass]))
	{
		//Replace this with StrNextToken
		i = 1;
		next = _tcschr(tokens[LdpListObjectClass], ';');
		while (next != NULL) {
			next = _tcschr(next + 1, ';');
			i++;
		}
		obj->computed.objectClassCount = i;
		obj->imported.objectClassesNames = ApiHeapAllocX(gs_hHeapLdapDump, (obj->computed.objectClassCount + 1) * sizeof(*(obj->imported.objectClassesNames)));

		i = 0;
		objectClassList = ApiStrDupX(gs_hHeapLdapDump, tokens[LdpListObjectClass]);
		obj->imported.objectClassesNames[0] = _tcstok_s(objectClassList, _T(";"), &next);
		while (obj->imported.objectClassesNames[i] != NULL) {
			i++;
			obj->imported.objectClassesNames[i] = _tcstok_s(NULL, _T(";"), &next);
		}
	}
	else
		obj->computed.objectClassCount = 0;

	if (!STR_EMPTY(tokens[LdpListObjectSid])) {
		API_LOG(Dbg, _T("Object <%s> has hex SID <%s>"), tokens[LdpListDn], tokens[LdpListObjectSid]);

		if (_tcslen(tokens[LdpListObjectSid]) / 2 > sizeof(obj->imported.sid)) {
			API_FATAL(_T("Hex sid <%s> too long"), tokens[LdpListObjectSid]);
		}
		api->Common.Unhexify((PBYTE)obj->imported.sid, tokens[LdpListObjectSid]);
		bResult = IsValidSid(obj->imported.sid);
		if (!bResult) {
			API_FATAL(_T("Invalid SID <%s> for <%s>"), tokens[LdpListObjectSid], tokens[LdpListDn]);
		}
		obj->computed.sidLength = GetLengthSid(obj->imported.sid);
	}
	api->InputCsv.CsvRecordArrayHeapFree(tokens, gs_ObjFileHeaderCount);
	return TRUE;
}


BOOL PLUGIN_IMPORTER_GETNEXTSCH(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_SCHEMA sch
) {
	BOOL bResult = FALSE;
	LPTSTR *tokens;
	DWORD i = 0;

	if (!gs_SchFileName) {
		API_FATAL(_T("reading SCH while 'ldpsch' option has not been specified"));
	}

	bResult = api->InputCsv.CsvGetNextRecord(gs_SchFile, &tokens, NULL);
	if (!bResult || !tokens) {
		// No more schema entries to import
		return FALSE;
	}
	// Lower case of every records, because of inconsistencies, i.e. gplinks
	for (i = 0; i < LDPDUMP_SCH_TOKEN_COUNT; i++) {
		if (!STR_EMPTY(tokens[i]))
			CharLower(tokens[i]);
	}
	sch->imported.dn = ApiStrDupX(gs_hHeapLdapDump, tokens[LdpSchDn]);
	sch->imported.defaultSecurityDescriptor = STR_EMPTY(tokens[LdpSchDefaultSecurityDescriptor]) ? 0 : ApiStrDupX(gs_hHeapLdapDump, tokens[LdpSchDefaultSecurityDescriptor]);
	sch->imported.lDAPDisplayName = STR_EMPTY(tokens[LdpSchLDAPDisplayName]) ? 0 : ApiStrDupX(gs_hHeapLdapDump, tokens[LdpSchLDAPDisplayName]);

	if (!STR_EMPTY(tokens[LdpSchSchemaIDGUID])) {
		size_t len = _tcslen(tokens[LdpSchSchemaIDGUID]);
		if (len != sizeof(GUID) * 2) {
			API_FATAL(_T("Invalid hex schema GUID <%s> (<len:%u> <expecting:%u>)"), tokens[LdpSchSchemaIDGUID], len, sizeof(GUID) * 2);
		}
		api->Common.Unhexify((PBYTE)&sch->imported.schemaIDGUID, tokens[LdpSchSchemaIDGUID]);
	}
	api->InputCsv.CsvRecordArrayHeapFree(tokens, gs_SchFileHeaderCount);
	return TRUE;
}

void PLUGIN_IMPORTER_FREEACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	ApiHeapFreeX(gs_hHeapLdapDump, ace->imported.objectDn);
	ApiHeapFreeX(gs_hHeapLdapDump, ace->imported.raw);
	RtlZeroMemory(ace, sizeof(IMPORTED_ACE));
}

void PLUGIN_IMPORTER_FREEOBJECT(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_OBJECT object
) {
	ApiHeapFreeX(gs_hHeapLdapDump, object->imported.dn);
	if (object->imported.mail) {
		ApiHeapFreeX(gs_hHeapLdapDump, object->imported.mail);
	}
	if (object->computed.objectClassCount) {
		ApiHeapFreeX(gs_hHeapLdapDump, object->imported.objectClassesNames[0]);
	}
	ApiHeapFreeX(gs_hHeapLdapDump, object->imported.objectClassesNames);
	RtlZeroMemory(object, sizeof(IMPORTED_OBJECT));
}

void PLUGIN_IMPORTER_FREESCHEMA(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_SCHEMA schema
) {
	ApiHeapFreeX(gs_hHeapLdapDump, schema->imported.dn);
	ApiHeapFreeX(gs_hHeapLdapDump, schema->imported.defaultSecurityDescriptor);
	ApiHeapFreeX(gs_hHeapLdapDump, schema->imported.lDAPDisplayName);
	LocalFree(schema->computed.defaultSD);
	RtlZeroMemory(schema, sizeof(IMPORTED_SCHEMA));
}
