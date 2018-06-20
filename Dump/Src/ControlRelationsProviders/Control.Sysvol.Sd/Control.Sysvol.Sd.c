/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_SYSVOL_OWNER_DEFAULT_OUTFILE    _T("control.sysvol.owner.csv")
#define CONTROL_SYSVOL_OWNER_KEYWORD            _T("SYSVOL_OWNER")

#define SYSVOL_DEFAULT_USE_BACKUP_PRIV          FALSE


/* --- TYPES ---------------------------------------------------------------- */ 
typedef struct _SYSVOL_OPTIONS {
    PTCHAR ptSysvolPoliciesPath;
    BOOL bUseBackupPriv;
} SYSVOL_OPTIONS, *PSYSVOL_OPTIONS;


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
SYSVOL_OPTIONS sSysvolOptions = { 0 };


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
BOOL SysvolParseGpoName(
    _In_ PTCHAR ptSysvolPoliciesPath,
    _In_ PTCHAR ptGpoCn,
    _Inout_ PTCHAR ptGpoPath
    ) {
    int size = -1;

    if (_tcslen(ptGpoCn) == (1 + STR_GUID_LEN + 1)) {
        if (ptGpoCn[0] == '{' && ptGpoCn[STR_GUID_LEN + 1] == '}') {
            size = _stprintf_s(ptGpoPath, MAX_PATH, _T("%s\\%s"), ptSysvolPoliciesPath, ptGpoCn);
            if (size != -1) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL SysvolFormatSubGpoElementName(
    _In_ PTCHAR ptSubElement,
    _In_ PTCHAR ptParentPath,
    _In_ PTCHAR ptSubPath,
    _In_opt_ PTCHAR ptParentDn,
    _Out_opt_ PTCHAR *ptSubDn
    ) {
    int size = -1;

    if (ptSubDn) {
        *ptSubDn = NULL;
    }

    size = _stprintf_s(ptSubPath, MAX_PATH, _T("%s\\%s"), ptParentPath, ptSubElement);
    if (size == -1) {
        return FALSE;
    }

    if (ptParentDn && ptSubDn) {
        size_t len = 3 + _tcslen(ptSubElement) + 1 + _tcslen(ptParentDn) + 1;
        *ptSubDn = malloc(len*sizeof(TCHAR));
        if (!*ptSubDn) {
			LOG(Err, _T("Unable to allocate ptSubDn structure"));
            return FALSE;
        }
        size = _stprintf_s(*ptSubDn, len, _T("CN=%s,%s"), ptSubElement, ptParentDn);
        if (size == -1) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL SysvolWriteOwner(
    _In_ CSV_HANDLE hOutfile,
    _In_ PTCHAR ptPath,
    _In_ PTCHAR ptDnSlave,
    _In_ PTCHAR ptKeyword
    ) {
    BOOL bResult = FALSE;
    DWORD dwResult = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    PSECURITY_DESCRIPTOR pSd = NULL;

    hFile = FileOpenWithBackupPriv(ptPath, sSysvolOptions.bUseBackupPriv);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG(Err, _T("Cannot open file <%s> : <%u>"), ptPath, GetLastError());
        return FALSE;
    }

    dwResult = GetSecurityInfo(hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &pSd);
    if (dwResult != ERROR_SUCCESS) {
        LOG(Err, _T("Cannot get security information for <%s> : <%u>"), ptPath, dwResult);
        return FALSE;
    }

    bResult = ControlWriteOwnerOutline(hOutfile, pSd, ptDnSlave, ptKeyword);
    if (!bResult) {
        LOG(Err, _T("Cannot write owner control relation for <%s>"), ptDnSlave);
        return FALSE;
    }

    return TRUE;
}

static void CallbackLdapGpoOwner(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
    ) {

    /*
    This currently checks :
    - the gpo root folder
    - the child folders 'Machine' and 'User'
    TODO : add other elements that can lead to a 'direct control relation' on the GPO (gpt.ini, registry.pol, etc.)
    */

    BOOL bResult = FALSE;
    PTCHAR ptGpoDn = NULL;
    PTCHAR ptGpoCn = NULL;
    PTCHAR ptGpoDnUser = NULL;
    PTCHAR ptGpoDnMachine = NULL;
    TCHAR tGpoPath[MAX_PATH] = { 0 };
    TCHAR tGpoPathUser[MAX_PATH] = { 0 };
    TCHAR tGpoPathMachine[MAX_PATH] = { 0 };

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (!tokens[LdpListObjectClass])
		return;

	if (_tcscmp(tokens[LdpListObjectClass], _T("top;container;grouppolicycontainer")))
		return;

	ptGpoDn = tokens[LdpListDn];
	ptGpoCn = tokens[LdpListCN];

    bResult = SysvolParseGpoName(sSysvolOptions.ptSysvolPoliciesPath, ptGpoCn, tGpoPath);
    if (!bResult) {
        LOG(Err, _T("Invalid GPO name <%s>"), ptGpoCn);
        return;
    }

    bResult = SysvolFormatSubGpoElementName(_T("User"), tGpoPath, tGpoPathUser, ptGpoDn, &ptGpoDnUser);
    if (!bResult) {
        LOG(Err, _T("Failed to format GPO sub element 'Machine'"));
        return;
    }

    bResult = SysvolFormatSubGpoElementName(_T("Machine"), tGpoPath, tGpoPathMachine, ptGpoDn, &ptGpoDnMachine);
    if (!bResult) {
        LOG(Err, _T("Failed to format GPO sub element 'User'"));
        return;
    }
	CharLower(ptGpoDn);
	CharLower(ptGpoDnUser);
	CharLower(ptGpoDnMachine);

    SysvolWriteOwner(hOutfile, tGpoPath, ptGpoDn, CONTROL_SYSVOL_OWNER_KEYWORD);
    SysvolWriteOwner(hOutfile, tGpoPathUser, ptGpoDnUser, CONTROL_SYSVOL_OWNER_KEYWORD);
    SysvolWriteOwner(hOutfile, tGpoPathMachine, ptGpoDnMachine, CONTROL_SYSVOL_OWNER_KEYWORD);

    if (ptGpoDnUser) {
        free(ptGpoDnUser);
    }
    if (ptGpoDnMachine) {
        free(ptGpoDnMachine);
    }
}

static void SysvolUsage(
    _In_ PTCHAR  progName
    ) {
    LOG(Bypass, _T("Usage : %s <general options> <sysvol options>"), progName);
    LOG(Bypass, _T("> general options :"));
    ControlUsage();
    LOG(Bypass, _T("> Sysvol options :"));
    LOG(Bypass, SUB_LOG(_T("-S <sysvol>   : path of the sysvol 'Policies' folder")));
    LOG(Bypass, SUB_LOG(_T("-B            : use 'SeBackupPrivilege' to access sysvol files")));

    ExitProcess(EXIT_FAILURE);
}

void SysvolParseOptions(
    _Inout_ PSYSVOL_OPTIONS pSysvolOptions,
    _In_ int argc,
    _In_ PTCHAR argv[]
    ) {
    int curropt = 0;

    pSysvolOptions->bUseBackupPriv = SYSVOL_DEFAULT_USE_BACKUP_PRIV;
//    optreset = 1;
    optind = 1;
    opterr = 0;

    while ((curropt = getopt(argc, argv, _T("BS:"))) != -1) {
        switch (curropt) {
        case _T('S'): pSysvolOptions->ptSysvolPoliciesPath = optarg; break;
        case _T('B'):
            pSysvolOptions->bUseBackupPriv = TRUE;
            if (!EnablePrivForCurrentProcess(SE_BACKUP_NAME)) {
                FATAL(_T("Cannot enable backup privilege for the current process"));
            }
            break;
        case _T('?'): optind++; break; // skipping unknown options
        }
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	bCacheBuilt = FALSE;
	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	RtlZeroMemory(&sSysvolOptions, sizeof(SYSVOL_OPTIONS));
	SysvolParseOptions(&sSysvolOptions, argc, argv);
	if (!sSysvolOptions.ptSysvolPoliciesPath) {
		SysvolUsage(argv[0]);
	}
	PTCHAR ptName = _T("SIDCACHE");
	CacheCreate(
		&ppCache,
		ptName,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildSidCache, SysvolUsage);
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackLdapGpoOwner, SysvolUsage);

	//
	// Cleanup
	//
	//WPP_CLEANUP();
	UtilsLibCleanup();
	CacheLibCleanup();
	CsvLibCleanup();
	LogLibCleanup();
	return EXIT_SUCCESS;
}
