/* --- INCLUDES ------------------------------------------------------------- */
#include "Control.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void ControlUsage(
    ) {
    LOG(Bypass, SUB_LOG(_T("-h/H          : show this help")));
    LOG(Bypass, SUB_LOG(_T("-O <outfile>  : outfile path")));
    LOG(Bypass, SUB_LOG(_T("-L <logfile>  : log output to a file (default is none)")));
    LOG(Bypass, SUB_LOG(_T("-D <dbglvl>   : set the log level. Possibles values are <ALL,DBG,INFO,WARN,ERR,SUCC,NONE> ")));
}

void ControlParseOptions(
    _Inout_ PCONTROL_OPTIONS pControlOptions,
    _In_ int argc,
    _In_ PTCHAR argv[]
    ) {
    int curropt = 0;
    
    pControlOptions->ptLogLevel = CONTROL_DEFAULT_LOGLVL;
    optreset = 1;
    optind = 1;
    opterr = 0;

    while ((curropt = getopt(argc, argv, _T("hHO:L:D:"))) != -1) {
        switch (curropt) {
        case _T('O'): pControlOptions->ptOutfile = optarg; break;
        case _T('L'): pControlOptions->ptLogFile = optarg; break;
        case _T('D'): pControlOptions->ptLogLevel = optarg; break;
        case _T('h'):
        case _T('H'): pControlOptions->bShowHelp = TRUE; break;
        case _T('?'): optind++; break; // skipping unknown options
        }
    }
}

HANDLE ControlOpenOutfile(
    _In_ PTCHAR ptOutfile
    ) {
    DWORD dwWritten = 0;
    HANDLE hOutfile = CreateFile(ptOutfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hOutfile != INVALID_HANDLE_VALUE) {
        WriteFile(hOutfile, CONTROL_OUTFILE_HEADER, (DWORD)_tcslen(CONTROL_OUTFILE_HEADER), &dwWritten, NULL);
    }
    return hOutfile;
}

BOOL ControlWriteOutline(
    _In_ HANDLE hOutfile,
    _In_ PTCHAR ptMaster,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    ) {
    TCHAR ptOutline[MAX_LINE] = { 0 };
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;
    int size = 0;

    size = _stprintf_s(ptOutline, _countof(ptOutline), CONTROL_OUTFILE_FORMAT, ptMaster, ptSlave, ptKeyword);
    if (size == -1) {
        LOG(Err, _T("Failed to format outline <%s : %s : %s>"), ptMaster, ptSlave, ptKeyword);
        return FALSE;
    }
    else {
        bResult = WriteFile(hOutfile, ptOutline, size, &dwWritten, NULL);
        if (!bResult) {
            LOG(Err, _T("Failed to write outline <%s : %s : %s>"), ptMaster, ptSlave, ptKeyword);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL ControlWriteOwnerOutline(
    _In_ HANDLE hOutfile,
    _In_ PSECURITY_DESCRIPTOR pSdOwner,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    ) {
    BOOL bResult = FALSE;
    BOOL bOwnerDefaulted = FALSE;
    PSID pSidOwner = NULL;
    PTCHAR ptDnOwner = NULL;

    bResult = GetSecurityDescriptorOwner(pSdOwner, &pSidOwner, &bOwnerDefaulted);
    if (!bResult) {
        LOG(Err, _T("Cannot get Owner from SD : <%u>"), GetLastError());
        return FALSE;
    }

    bResult = LdapResolveRawSid(pSidOwner, &ptDnOwner);
    if (!bResult) {
        LOG(Warn, _T("Unresolved owner SID : <%s>"), ptDnOwner);
        return FALSE;
    }

    bResult = ControlWriteOutline(hOutfile, ptDnOwner, ptSlave, ptKeyword);
    free(ptDnOwner);

    return bResult;
}

void ControlMainForeachLdapResult(
    _In_ int argc,
    _In_ PTCHAR argv[],
    _In_ PTCHAR ptDefaultOutfile,
    _In_ PTCHAR ptLdapFilter,
    _In_ PTCHAR ptAttribute,
    _In_opt_ PLDAPControl *pLdapServerControls,
    _In_ FN_CONTROL_CALLBACK_LDAP_RESULT pfnCallback,
    _In_ FN_USAGE_CALLBACK pfnUsage
    ) {

    /* Variables */
    BOOL bResult = FALSE;
    DWORD i = 0;
    LDAP_OPTIONS sLdapOptions = { 0 };
    PLDAP_RETRIEVED_DATA *pRetrievedResults = NULL;
    DWORD dwResultsCount = 0;
    HANDLE hOutfile = INVALID_HANDLE_VALUE;
    CONTROL_OPTIONS sControlOptions = { 0 };

    /* Options parsing */
    RtlZeroMemory(&sLdapOptions, sizeof(LDAP_OPTIONS));
    RtlZeroMemory(&sControlOptions, sizeof(CONTROL_OPTIONS));
    sControlOptions.ptOutfile = ptDefaultOutfile;
    LdapParseOptions(&sLdapOptions, argc, argv);
    ControlParseOptions(&sControlOptions, argc, argv);

    if (sControlOptions.ptLogFile)  SetLogFile(sControlOptions.ptLogFile);
    if (sControlOptions.ptLogLevel) SetLogLevel(sControlOptions.ptLogLevel);
    if (sControlOptions.bShowHelp || !sLdapOptions.tLDAPServer) {
        pfnUsage(argv[0]);
    }

    LOG(Succ, _T("Starting"));
    LOG(Info, _T("outfile is <%s>"), sControlOptions.ptOutfile);

    /* Outfile opening */
    LOG(Info, _T("Opening outfile <%s>"), sControlOptions.ptOutfile);
    hOutfile = ControlOpenOutfile(sControlOptions.ptOutfile);
    if (hOutfile == INVALID_HANDLE_VALUE) {
        FATAL(_T("Failed to initialize outfile <%s> : <GLE=%u>"), sControlOptions.ptOutfile, GetLastError());
    }


    /* LDAP Init/Connect/Bind */
    LOG(Info, _T("Connecting to domain controller <%s>"), sLdapOptions.tLDAPServer);
    bResult = LdapInitAndBind(&sLdapOptions);
    if (!bResult) {
        FATAL(_T("Unable to init LDAP connection to domain controller"));
    }


    /* LDAP Query */
    LOG(Info, _T("Sending LDAP query"));
    bResult = LdapPagedSearch(ptLdapFilter, ptAttribute, pLdapServerControls, &pRetrievedResults, &dwResultsCount);
    if (!bResult) {
        FATAL(_T("Failed LDAP request : <%s>"), ptLdapFilter);
    }


    /* Results parsing  */
    LOG(Info, _T("Parsing query results <%u>"), dwResultsCount);

    for (i = 0; i < dwResultsCount; i++) {
        LOG(Dbg, SUB_LOG(_T("%s :")), pRetrievedResults[i]->tDN);
        pfnCallback(hOutfile, pRetrievedResults[i]);
    }


    /* Done */
    LdapFreePagedSearchResults(&pRetrievedResults, &dwResultsCount);
    CloseHandle(hOutfile);
    LdapDisconnectAndDestroy();
    LOG(Succ, _T("Done"));
}
