/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("LdapDump")
#define PLUGIN_KEYWORD      _T("LDP")
#define PLUGIN_DESCRIPTION  _T("Imports ACE, OBJ and SCH from LdapDump's oufiles (ace/obj/sch.ldpdmp.tsv)");

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


/* --- DEFINES -------------------------------------------------------------- */
#define LDPDUMP_ACE_TOKEN_COUNT         (2)
#define LDPDUMP_SCHEMA_TOKEN_COUNT      (5)
#define LDPDUMP_LIST_TOKEN_COUNT        (4)

#define LDPDUMP_HEADER_FIRST_TOKEN      _T("dn")

#define OID_MS_AD_CLASS_SUBSTR          _T("1.2.840.113556.1.5.")


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
} OBJ_TSV_TOKENS;


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_AceFileName = NULL;
static LPTSTR gs_ObjFileName = NULL;
static LPTSTR gs_SchFileName = NULL;

static INPUT_FILE gs_AceFile = { 0 };
static INPUT_FILE gs_ObjFile = { 0 };
static INPUT_FILE gs_SchFile = { 0 };

static LONGLONG gs_AcePosition = 0;
static LONGLONG gs_ObjPosition = 0;
static LONGLONG gs_SchPosition = 0;

static PVOID gs_AceMapping = NULL;
static PVOID gs_ObjMapping = NULL;
static PVOID gs_SchMapping = NULL;

static LPTSTR gs_AceTokens[LDPDUMP_ACE_TOKEN_COUNT] = { 0 };
static PSECURITY_DESCRIPTOR gs_CurrentSd = NULL;
static PACL gs_CurrentDacl = NULL;
static DWORD gs_CurrentAceIndex = 0;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void OpenInputFileAndSkipHeader(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ PINPUT_FILE inputFile,
    _In_ LPTSTR path,
    _In_ LPTSTR name,
    _Inout_ PVOID *mapping,
    _Inout_ PLONGLONG position
    ) {
    BOOL bResult = FALSE;

    // Open input file
    bResult = api->InputFile.InitInputFile(path, name, inputFile);
    if (!bResult) {
        API_FATAL(_T("Failed to open input file <%s>"), path);
    }
    else {
        API_LOG(Dbg, _T("Successfully opened file <%s>"), path);
    }

    api->InputFile.ResetInputFile(inputFile, mapping, position);
}

static void ResetAceVars(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    if (gs_CurrentSd) {
        ApiLocalFreeCheckX(gs_CurrentSd);
    }
    gs_CurrentSd = NULL;
    gs_CurrentDacl = NULL;
    gs_CurrentAceIndex = 0;
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
    gs_AceFileName = api->Common.GetPluginOption(_T("ldpace"), FALSE);
    gs_ObjFileName = api->Common.GetPluginOption(_T("ldpobj"), FALSE);
    gs_SchFileName = api->Common.GetPluginOption(_T("ldpsch"), FALSE);

    if (!gs_AceFileName && !gs_ObjFileName && !gs_SchFileName) {
        API_FATAL(_T("You must specify at least one plugin option between 'ldpace', 'ldpobj' and 'ldpsch'"));
    }
    API_LOG(Info, _T("Infile ace is <%s>"), gs_AceFileName);
    API_LOG(Info, _T("Infile obj is <%s>"), gs_ObjFileName);
    API_LOG(Info, _T("Infile sch is <%s>"), gs_SchFileName);

    if (gs_AceFileName) OpenInputFileAndSkipHeader(api, &gs_AceFile, gs_AceFileName, _T("LDPACE"), &gs_AceMapping, &gs_AcePosition);
    if (gs_ObjFileName) OpenInputFileAndSkipHeader(api, &gs_ObjFile, gs_ObjFileName, _T("LDPLIST"), &gs_ObjMapping, &gs_ObjPosition);
    if (gs_SchFileName) OpenInputFileAndSkipHeader(api, &gs_SchFile, gs_SchFileName, _T("LDPSCHEMA"), &gs_SchMapping, &gs_SchPosition);

    ResetAceVars(api);

    return TRUE;
}


BOOL PLUGIN_GENERIC_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    if (gs_AceFileName) api->InputFile.CloseInputFile(&gs_AceFile);
    if (gs_ObjFileName) api->InputFile.CloseInputFile(&gs_ObjFile);
    if (gs_SchFileName) api->InputFile.CloseInputFile(&gs_SchFile);

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
        api->InputFile.ResetInputFile(&gs_AceFile, &gs_AceMapping, &gs_AcePosition);
        ResetAceVars(api);
        break;

    case ImporterObject:
        if (!gs_ObjFileName) {
            API_FATAL(_T("Plugin option <ldpobj> not specified, but requesting OBJ operation (read-reset)"));
        }
        api->InputFile.ResetInputFile(&gs_ObjFile, &gs_ObjMapping, &gs_ObjPosition);
        break;

    case ImporterSchema:
        if (!gs_SchFileName) {
            API_FATAL(_T("Plugin option <ldpsch> not specified, but requesting SCH operation (read-reset)"));
        }
        api->InputFile.ResetInputFile(&gs_SchFile, &gs_SchMapping, &gs_SchPosition);
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

    if (!gs_AceFileName) {
        API_FATAL(_T("reading ACE while 'ldpace' option has not been specified"));
    }

    if (!gs_CurrentDacl || gs_CurrentAceIndex >= gs_CurrentDacl->AceCount) {
        // get the next DACL
        API_LOG(Dbg, _T("Getting next DACL..."));
        ResetAceVars(api);
        bResult = api->InputFile.ReadParseTsvLine((PBYTE *)&gs_AceMapping, gs_AceFile.fileSize.QuadPart, &gs_AcePosition, LDPDUMP_ACE_TOKEN_COUNT, gs_AceTokens, LDPDUMP_HEADER_FIRST_TOKEN);
        if (!bResult) {
            return FALSE; // No more DACL => No more ACE to import
        }
        else {
            BOOL bDaclPresent = FALSE, bDaclDefaulted = FALSE;
            size_t len = _tcslen(gs_AceTokens[LdpAceNtSecurityDescriptor]);
            
            gs_CurrentSd = ApiLocalAllocCheckX(len / 2);
            api->Common.Unhexify(gs_CurrentSd, gs_AceTokens[LdpAceNtSecurityDescriptor]);
            bResult = IsValidSecurityDescriptor(gs_CurrentSd);
            if (!bResult) {
                API_FATAL(_T("Invalid SD for <%s>"), gs_AceTokens[LdpAceDn]);
            }

            bResult = GetSecurityDescriptorDacl(gs_CurrentSd, &bDaclPresent, &gs_CurrentDacl, &bDaclDefaulted);
            if (!bResult) {
                API_FATAL(_T("Cannot get DACL for <%s> : <%u>"), gs_AceTokens[LdpAceDn], GetLastError());
            }
            if (!bDaclPresent) {
                API_FATAL(_T("No DACL present for <%s>"), gs_AceTokens[LdpAceDn]);
            }
        }
    }

    API_LOG(Dbg, _T("Getting ACE <%u/%u> for <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_AceTokens[LdpAceDn]);

    // Get the next ACE in the current DACL and inc the index
    bResult = GetAce(gs_CurrentDacl, gs_CurrentAceIndex, &currentAce);
    if (!bResult) {
        API_FATAL(_T("Cannot get ACE <%u/%u> of <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_AceTokens[LdpAceDn]);
    }

    // Import the ACE
    ace->imported.source = AceFromActiveDirectory;
    ace->imported.raw = ApiLocalAllocCheckX(currentAce->AceSize);
    memcpy(ace->imported.raw, currentAce, currentAce->AceSize);
    ace->imported.objectDn = ApiStrDupCheckX(gs_AceTokens[LdpAceDn]);
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
    LPTSTR tokens[LDPDUMP_LIST_TOKEN_COUNT] = { 0 };
	DWORD i;
	LPTSTR next;

    if (!gs_ObjFileName) {
        API_FATAL(_T("reading OBJ while 'ldpobj' option has not been specified"));
    }

    bResult = api->InputFile.ReadParseTsvLine((PBYTE *)&gs_ObjMapping, gs_ObjFile.fileSize.QuadPart, &gs_ObjPosition, LDPDUMP_LIST_TOKEN_COUNT, tokens, LDPDUMP_HEADER_FIRST_TOKEN);
    if (!bResult) {
        return FALSE;
    }
    else {
        obj->imported.dn = ApiStrDupCheckX(tokens[LdpListDn]);
        obj->imported.adminCount = STR_EMPTY(tokens[LdpListAdminCount]) ? 0 : 1;

		
		i = 1;
		next = strchr(tokens[LdpListObjectClass], ',');
		while (next != NULL) {
			next = strchr(next + 1, ',');
			i++;
		}
		obj->computed.objectClassCount = i;
		obj->imported.objectClassesNames = ApiLocalAllocCheckX((obj->computed.objectClassCount + 1) * sizeof(*(obj->imported.objectClassesNames)));
		
		i = 0;
		obj->imported.objectClassesNames[0] = strtok_s(ApiStrDupCheckX(tokens[LdpListObjectClass]), ",", &next);
		while (obj->imported.objectClassesNames[i] != NULL) {
			i++ ;
			obj->imported.objectClassesNames[i] = strtok_s(NULL, ",", &next);
		}
		

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
        return TRUE;
    }
}


BOOL PLUGIN_IMPORTER_GETNEXTSCH(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_SCHEMA sch
    ) {
    BOOL bResult = FALSE;
    LPTSTR tokens[LDPDUMP_SCHEMA_TOKEN_COUNT] = { 0 };

    if (!gs_SchFileName) {
        API_FATAL(_T("reading SCH while 'ldpsch' option has not been specified"));
    }

    bResult = api->InputFile.ReadParseTsvLine((PBYTE *)&gs_SchMapping, gs_SchFile.fileSize.QuadPart, &gs_SchPosition, LDPDUMP_SCHEMA_TOKEN_COUNT, tokens, LDPDUMP_HEADER_FIRST_TOKEN);
    if (!bResult) {
        return FALSE;
    }
    else {
        sch->imported.dn = ApiStrDupCheckX(tokens[LdpSchDn]);
        sch->imported.defaultSecurityDescriptor = ApiStrDupCheckX(tokens[LdpSchDefaultSecurityDescriptor]);
		sch->imported.lDAPDisplayName = ApiStrDupCheckX(tokens[LdpSchLDAPDisplayName]);

        if (!STR_EMPTY(tokens[LdpSchGovernsID])) {
            // governsID is given in the form of an OID. What we check here is only if the OID corresponds to a MS AD class (starts with "1.2.840.113556.1.5.")
            // cf. http://msdn.microsoft.com/en-us/library/windows/desktop/ms677614(v=vs.85).aspx
            LPTSTR msAdClassOid = _tcsstr(tokens[LdpSchGovernsID], OID_MS_AD_CLASS_SUBSTR);
            if (msAdClassOid == tokens[LdpSchGovernsID]) {
                sch->imported.governsID = _tstoi(msAdClassOid + _tcslen(OID_MS_AD_CLASS_SUBSTR));
            }
        }

        if (!STR_EMPTY(tokens[LdpSchSchemaIDGUID])) {
            size_t len = _tcslen(tokens[LdpSchSchemaIDGUID]);
            if (len != sizeof(GUID)* 2) {
                API_FATAL(_T("Invalid hex schema GUID <%s> (<len:%u> <expecting:%u>)"), tokens[LdpSchSchemaIDGUID], len, sizeof(GUID)* 2);
            }
            api->Common.Unhexify((PBYTE)&sch->imported.schemaIDGUID, tokens[LdpSchSchemaIDGUID]);
        }

        return TRUE;
    }
}
