/******************************************************************************\
\******************************************************************************/


/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Sysvol")
#define PLUGIN_KEYWORD      _T("SVL")
#define PLUGIN_DESCRIPTION  _T("Imports only ACE from the SYSVOL (may be a Robocopy). OBJ and SCH must come from another importer.");
#define SYSVOL_HEAP_NAME    _T("SYSVOLHEAP")

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FINALIZE;
PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_GETNEXTACE;
PLUGIN_DECLARE_RESETREADING;
PLUGIN_DECLARE_FREEACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION); // for api->Resolver.GetDomainDn();


/* --- DEFINES -------------------------------------------------------------- */
#define SYSVOL_PATH_GPO_WILDCARD    _T("%s\\{????????-????-????-????-????????????}")
#define SYSVOL_POLICIES_SUB_DN      _T("CN=Policies,CN=System,")


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _GPO_ACTION {
    GpoInit,
    GpoRoot,
    GpoMachine,
    GpoUser,
} GPO_ACTION;


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PUTILS_HEAP gs_hHeapSysvol = INVALID_HANDLE_VALUE;
static BOOL gs_UseBackupPriv = FALSE;
static TCHAR gs_SysvolPoliciesSearchPath[MAX_PATH] = { 0 };
static LPTSTR gs_SysvolPoliciesPath = NULL;

static HANDLE gs_FindHandle = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATA gs_CurrentFileData = { 0 };

static GPO_ACTION gs_NextAction = GpoRoot;
static LPTSTR gs_PoliciesDn = NULL;
static LPTSTR gs_CurrentGpoDn = NULL;
static LPTSTR gs_CurrentElementDn = NULL;
static TCHAR gs_CurrentGpoPath[MAX_PATH] = { 0 };
static TCHAR gs_CurrentElementPath[MAX_PATH] = { 0 };

static PSECURITY_DESCRIPTOR gs_CurrentSd = NULL;
static PACL gs_CurrentDacl = NULL;
static DWORD gs_CurrentAceIndex = 0;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static PACL SysvolGetGpoElementDacl(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ LPTSTR filePath
    ) {
    BOOL bDaclPresent = FALSE;
    BOOL bDaclDefaulted = FALSE;
    BOOL bResult = FALSE;
    DWORD dwResult = 0;
    HANDLE hFile = FileOpenWithBackupPriv(filePath, gs_UseBackupPriv);
    if (hFile == INVALID_HANDLE_VALUE) {
        API_LOG(Err, _T("Failed to open <%s> : <%u>"), filePath, GetLastError());
        return NULL;
    }

    dwResult = GetSecurityInfo(hFile, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &gs_CurrentSd);
    if (dwResult != ERROR_SUCCESS) {
        API_LOG(Err, _T("Cannot get security information for <%s> : <%u>"), filePath, dwResult);
        return NULL;
    }

    bResult = GetSecurityDescriptorDacl(gs_CurrentSd, &bDaclPresent, &gs_CurrentDacl, &bDaclDefaulted);
    if (!bResult) {
        LOG(Err, _T("Cannot get DACL for <%s> : <%u>"), filePath, GetLastError());
        return NULL;
    }
    if (!bDaclPresent) {
        LOG(Err, _T("No DACL present for <%s>"), filePath);
        return NULL;
    }

    return gs_CurrentDacl;
}


static BOOL SysvolValidGpoName(
    _In_ LPTSTR name
    ) {
    // Meh ... better than nothing...
    return (BOOL)(
        _tcslen(name) == GUID_STR_SIZE &&
        name[0] == _T('{') &&
        name[GUID_STR_SIZE - 1] == _T('}') &&
        name[9] == _T('-') &&
        name[14] == _T('-') &&
        name[19] == _T('-') &&
        name[24] == _T('-')
        );
}


BOOL SysvolFormatSubGpoElementName(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ PTCHAR ptSubElement,
    _In_ PTCHAR ptParentPath,
    _In_ PTCHAR ptSubPath,
    _In_opt_ PTCHAR ptParentDn,
    _In_opt_ PTCHAR *ptSubDn
    ) {
    int size = -1;

    size = _stprintf_s(ptSubPath, MAX_PATH, _T("%s\\%s"), ptParentPath, ptSubElement);
    if (size == -1) {
        return FALSE;
    }

    if (ptParentDn && ptSubDn) {
        size_t len = 3 + _tcslen(ptSubElement) + 1 + _tcslen(ptParentDn) + 1;
        *ptSubDn = ApiHeapAllocX(gs_hHeapSysvol, (DWORD)(len*sizeof(TCHAR)));
        if (!*ptSubDn) {
            return FALSE;
        }
        size = _stprintf_s(*ptSubDn, len, _T("CN=%s,%s"), ptSubElement, ptParentDn);
        if (size == -1) {
            return FALSE;
        }
    }
    return TRUE;
}


static void SysvolGetPoliciesDn(
    _In_ PLUGIN_API_TABLE const * const api
    ){
    int szPoliciesDn = 0;
	size_t len = 0;
    LPTSTR domain = api->Resolver.GetDomainDn();

    if (!domain) {
        API_FATAL(_T("Failed to get domain DN"));
    }
    len = _tcslen(SYSVOL_POLICIES_SUB_DN) + _tcslen(domain) + 1;
    gs_PoliciesDn = ApiHeapAllocX(gs_hHeapSysvol, (DWORD)(len*sizeof(TCHAR)));
	szPoliciesDn = _stprintf_s(gs_PoliciesDn, len, _T("cn=policies,cn=system,%s"), domain);
    if (!szPoliciesDn) {
        API_FATAL(_T("Failed to format 'policies' DN"));
    }
}


static BOOL SysvolGetNextDacl(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;
    gs_CurrentDacl = NULL;
    gs_CurrentAceIndex = 0;

    if (!gs_PoliciesDn) {
        SysvolGetPoliciesDn(api);
    }

    while (gs_CurrentDacl == NULL) {
        switch (gs_NextAction) {


            // Read next filename ...
        case GpoRoot:
            bResult = FindNextFile(gs_FindHandle, &gs_CurrentFileData);
            if (!bResult) {
                DWORD gle = GetLastError();
                if (gle != ERROR_NO_MORE_FILES) {
                    API_FATAL(_T("Error while listing sysvol GPO folders : <%u>"), gle);
                }
                return FALSE;
            }
            // Notice no 'break' here.


            // ... and get GPO DN from it
        case GpoInit:
            bResult = SysvolValidGpoName(gs_CurrentFileData.cFileName);
            if (!bResult) {
                API_FATAL(_T("Invalid filename <%s>"), gs_CurrentFileData.cFileName);
            }
            bResult = SysvolFormatSubGpoElementName(api, gs_CurrentFileData.cFileName, gs_SysvolPoliciesPath, gs_CurrentGpoPath, gs_PoliciesDn, &gs_CurrentGpoDn);
            _tcscpy_s(gs_CurrentElementPath, MAX_PATH, gs_CurrentGpoPath);
            gs_CurrentElementDn = gs_CurrentGpoDn;
            if (!bResult) {
                API_FATAL(_T("Failed to format GPO 'root' names"));
            }
            gs_NextAction = GpoMachine;
            break;


            // Get 'Machine' Sub-GPO-element DN from GPO DN
        case GpoMachine:
            bResult = SysvolFormatSubGpoElementName(api, _T("Machine"), gs_CurrentGpoPath, gs_CurrentElementPath, gs_CurrentGpoDn, &gs_CurrentElementDn);
            if (!bResult) {
                API_FATAL(_T("Failed to format GPO sub element 'Machine' names"));
            }
            gs_NextAction = GpoUser;
            break;


            // Get 'User' Sub-GPO-element DN from GPO DN
        case GpoUser:
            bResult = SysvolFormatSubGpoElementName(api, _T("User"), gs_CurrentGpoPath, gs_CurrentElementPath, gs_CurrentGpoDn, &gs_CurrentElementDn);
            if (!bResult) {
                API_FATAL(_T("Failed to format GPO sub element 'User' names"));
            }
            gs_NextAction = GpoRoot;
            break;


            // Error
        default:
            API_FATAL(_T("Invalid GPO current state <%u>"), gs_NextAction);
        }

        API_LOG(Dbg, _T("Getting Dacl of <%s>"), gs_CurrentElementPath);
        gs_CurrentDacl = SysvolGetGpoElementDacl(api, gs_CurrentElementPath);
        if (gs_CurrentDacl == NULL) {
            API_LOG(Err, _T("Cannot get DACL for <%s> : <%u>"), gs_CurrentElementPath, GetLastError());
        }
    }

    return TRUE;
}


static void SysvolInitAceReading(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    if (gs_FindHandle != INVALID_HANDLE_VALUE) {
        FindClose(gs_FindHandle);
    }

    gs_FindHandle = FindFirstFile(gs_SysvolPoliciesSearchPath, &gs_CurrentFileData);
    if (gs_FindHandle == INVALID_HANDLE_VALUE) {
        API_FATAL(_T("Cannot init directory listing for <%s>"), gs_SysvolPoliciesPath);
    }

    gs_NextAction = GpoInit;
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the infile with the <sysvol> plugin option (no default)"));
    API_LOG(Bypass, _T("This plugin can only import ACE. You must combine it with others importers that import OBJ and SCH"));
    API_LOG(Bypass, _T("When using multiple importers, this one must be placed first in the command-line"));
    API_LOG(Bypass, _T("You must use the <usebackpriv> plugin option to use the 'SeBackupPrivilege> when accessing a Robocopy"));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
	BOOL bResult = FALSE;

	bResult = ApiHeapCreateX(&gs_hHeapSysvol, SYSVOL_HEAP_NAME, NULL);
	if (API_FAILED(bResult)) {
		return ERROR_VALUE;
	}
    // Backup priv is implicitely used by FindFirst/NextFile
    gs_UseBackupPriv = api->Common.GetPluginOption(_T("usebackpriv"), FALSE) != NULL ? TRUE : FALSE;
    API_LOG(Info, _T("Accessing sysvol %s backup privilege"), gs_UseBackupPriv ? _T("using") : _T("not using"));
    if (gs_UseBackupPriv) {
        if (!api->Common.EnablePrivForCurrentProcess(SE_BACKUP_NAME)) {
            API_FATAL(_T("Cannot enable backup privilege for the current process"));
        }
    }

    gs_SysvolPoliciesPath = api->Common.GetPluginOption(_T("sysvol"), TRUE);
    int size = -1;

    size = _stprintf_s(gs_SysvolPoliciesSearchPath, MAX_PATH, SYSVOL_PATH_GPO_WILDCARD, gs_SysvolPoliciesPath);
    if (size == -1) {
        API_FATAL(_T("Failed to format sysvol 'Policies' path"));
    }

    API_LOG(Dbg, _T("Sysvol 'Policies' path <%s> (search path <%s>)"), gs_SysvolPoliciesPath, gs_SysvolPoliciesSearchPath);

    SysvolInitAceReading(api);

    return TRUE;
}


BOOL PLUGIN_GENERIC_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
	BOOL bResult = FALSE;
	bResult = ApiHeapDestroyX(&gs_hHeapSysvol);
	if (API_FAILED(bResult)) {
		return ERROR_VALUE;
	}
    FindClose(gs_FindHandle);
    return TRUE;
}


void PLUGIN_IMPORTER_RESETREADING(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ IMPORTER_DATATYPE type
    ) {
    if (type != ImporterAce) {
        API_FATAL(_T("Unsupported read-reset operation <%u>"), type);
    }
    else {
        SysvolInitAceReading(api);
    }
}


BOOL PLUGIN_IMPORTER_GETNEXTACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    BOOL bResult = TRUE;
    PACE_HEADER currentAce = NULL;

    if (!gs_CurrentDacl || gs_CurrentAceIndex >= gs_CurrentDacl->AceCount) {
        bResult = SysvolGetNextDacl(api);
        if (!bResult || !gs_CurrentDacl) {
            return FALSE; // No more DACL => No more ACE to import
        }
    }

    API_LOG(Dbg, _T("Getting ACE <%u/%u> for <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_CurrentElementPath);

    // Get the next ACE in the current DACL and inc the index
    bResult = GetAce(gs_CurrentDacl, gs_CurrentAceIndex, &currentAce);
    if (!bResult) {
        API_FATAL(_T("Cannot get ACE <%u/%u> of <%s>"), gs_CurrentAceIndex + 1, gs_CurrentDacl->AceCount, gs_CurrentElementPath);
    }

    // Import the ACE
    ace->imported.source = AceFromFileSystem;
    ace->imported.raw = ApiHeapAllocX(gs_hHeapSysvol,currentAce->AceSize);
    memcpy(ace->imported.raw, currentAce, currentAce->AceSize);
    ace->imported.objectDn = ApiStrDupX(gs_hHeapSysvol,gs_CurrentElementDn);
	CharLower(ace->imported.objectDn);
    if (!IsValidSid(api->Ace.GetTrustee(ace))) {
        API_FATAL(_T("ace does not have a valid trustee sid <%s>"), ace->imported.objectDn);
    }
    gs_CurrentAceIndex++;

    return TRUE;
}

void PLUGIN_IMPORTER_FREEACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
) {
	ApiHeapFreeX(gs_hHeapSysvol, ace->imported.objectDn);
	ApiHeapFreeX(gs_hHeapSysvol, ace->imported.raw);
	RtlZeroMemory(ace, sizeof(IMPORTED_ACE));
}
