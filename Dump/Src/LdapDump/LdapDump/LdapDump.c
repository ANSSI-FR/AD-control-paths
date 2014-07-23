/* --- INCLUDES ------------------------------------------------------------- */
#include "LdapDump.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static const LPTSTR gsc_aszAttrNamesAce[LDAP_DUMP_ATTR_COUNT_ACE] = {
    [AttrAce_ntSecurityDescriptor] = LDAP_ATTR_NTSD,
};

static const LDAP_DUMP_ATTR_FORMAT gsc_aeAttrFormatsAce[LDAP_DUMP_ATTR_COUNT_ACE] = {
    [AttrAce_ntSecurityDescriptor] = AttrHex,
};

static const LPTSTR gsc_aszAttrNamesObj[LDAP_DUMP_ATTR_COUNT_OBJ] = {
    [AttrObj_objectSid] = LDAP_ATTR_OBJECT_SID,
    [AttrObj_objectClass] = LDAP_ATTR_OBJECT_CLASS,
    [AttrObj_adminCount] = LDAP_ATTR_ADMIN_COUNT,
};

static const LDAP_DUMP_ATTR_FORMAT gsc_aeAttrFormatsObj[LDAP_DUMP_ATTR_COUNT_OBJ] = {
    [AttrObj_objectClass] = AttrStr,
    [AttrObj_objectSid] = AttrHex,
    [AttrObj_adminCount] = AttrStr,
};

static const LPTSTR gsc_aszAttrNamesSch[LDAP_DUMP_ATTR_COUNT_SCH] = {
    [AttrSch_schemaIDGUID] = LDAP_ATTR_SCHEMA_IDGUID,
    [AttrSch_governsID] = LDAP_ATTR_GOVERNS_ID,
    [AttrSch_defaultSecurityDescriptor] = LDAP_ATTR_DEFAULT_SD,
};

static const LDAP_DUMP_ATTR_FORMAT gsc_aeAttrFormatsSch[LDAP_DUMP_ATTR_COUNT_SCH] = {
    [AttrSch_schemaIDGUID] = AttrHex,
    [AttrSch_governsID] = AttrStr,
    [AttrSch_defaultSecurityDescriptor] = AttrStr,
};


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void Usage(
    _In_        LPCTSTR  progName,
    _In_opt_    LPCTSTR  msg
    ) {
    if (msg) {
        LOG(Err, _T("Error: %s"), msg);
    }
    LOG(Bypass, _T("Usage: %s <ldap options> <dump options> <misc options>"), progName);

    LOG(Bypass, _T("Ldap options:"));
    LOG(Bypass, SUB_LOG(_T("-s <server>   : ldap server to dump information from")));
    LOG(Bypass, SUB_LOG(_T("-l <username> : AD username for explicit authentification")));
    LOG(Bypass, SUB_LOG(_T("-p <password> : AD password for explicit authentification")));
    LOG(Bypass, SUB_LOG(_T("-n <port>     : ldap port (default: <%u>)")), DEFAULT_LDAP_PORT);
    LOG(Bypass, SUB_LOG(_T("-d <dns name> : domain dns name (default: resolved dynamically)")));

    LOG(Bypass, _T("Dump options:"));
    LOG(Bypass, SUB_LOG(_T("-A/O/C        : dump Ace/Object/Schema list (all if none specified)")));
    LOG(Bypass, SUB_LOG(_T("-a <ace file> : Ace TSV outfile (default: <%s>)")), DEFAULT_OPT_OUTFILE_ACE);
    LOG(Bypass, SUB_LOG(_T("-o <obj file> : Obj TSV outfile (default: <%s>)")), DEFAULT_OPT_OUTFILE_OBJECT);
    LOG(Bypass, SUB_LOG(_T("-c <sch file> : Sch TSV outfile (default: <%s>)")), DEFAULT_OPT_OUTFILE_SCHEMA);
    LOG(Bypass, SUB_LOG(_T("-D <dn>       : Restrict dump to a particular DN (default: dump for domain, schema and config NC)")));

    LOG(Bypass, _T("Misc options:"));
    LOG(Bypass, SUB_LOG(_T("-h/H          : Show this help")));
    LOG(Bypass, SUB_LOG(_T("-x <level>    : Set log level. Possibles values are <ALL,DBG,INFO,WARN,ERR,SUCC,NONE>")));
    LOG(Bypass, SUB_LOG(_T("-f <logfile>  : Log output to a file (default is none)")));

    ExitProcess(EXIT_FAILURE);
}

static void ParseOptions(
    _In_ PLDAP_DUMP_OPTIONS pOpt,
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    int curropt = 0;
    PTCHAR ptSlashInLogin = NULL;

    pOpt->outfiles.szOutfileAce = DEFAULT_OPT_OUTFILE_ACE;
    pOpt->outfiles.szOutfileObjectList = DEFAULT_OPT_OUTFILE_OBJECT;
    pOpt->outfiles.szOutfileSchema = DEFAULT_OPT_OUTFILE_SCHEMA;
    pOpt->ldap.dwLDAPPort = DEFAULT_LDAP_PORT;
    pOpt->misc.szLogLevel = DEFAULT_OPT_LOG_LEVEL;

    while ((curropt = getopt(argc, argv, _T("s:l:p:n:d:Aa:Oo:Cc:D:Hhx:f:"))) != -1) {
        switch (curropt) {

        case _T('s'): pOpt->ldap.tLDAPServer = optarg; break;
        case _T('l'): pOpt->ldap.tADLogin = optarg; break;
        case _T('p'): pOpt->ldap.tADPassword = optarg; break;
        case _T('n'): pOpt->ldap.dwLDAPPort = _tstoi(optarg); break;
        case _T('d'): pOpt->ldap.tDNSName = optarg; break;

        case _T('A'): pOpt->actions.bDumpAce = TRUE; break;
        case _T('O'): pOpt->actions.bDumpObjectList = TRUE; break;
        case _T('C'): pOpt->actions.bDumpSchema = TRUE; break;

        case _T('a'): pOpt->outfiles.szOutfileAce = optarg; break;
        case _T('o'): pOpt->outfiles.szOutfileObjectList = optarg; break;
        case _T('c'): pOpt->outfiles.szOutfileSchema = optarg; break;

        case _T('D'): pOpt->misc.szRestrictDumpDn = optarg; break;

        case _T('h'):
        case _T('H'): pOpt->misc.bShowHelp = TRUE; break;
        case _T('x'): pOpt->misc.szLogLevel = optarg; break;
        case _T('f'): pOpt->misc.szLogFile = optarg; break;

        default:
            FATAL(_T("Unknown option"));
        }
    }

    if (!pOpt->actions.bDumpAce && !pOpt->actions.bDumpObjectList && !pOpt->actions.bDumpSchema) {
        pOpt->actions.bDumpAce = pOpt->actions.bDumpObjectList = pOpt->actions.bDumpSchema = TRUE;
    }

    if (pOpt->ldap.tADLogin != NULL) {
        ptSlashInLogin = _tcschr(pOpt->ldap.tADLogin, _T('\\'));
        if (ptSlashInLogin != NULL) {
            // userName actually starts with the domain name (ex: DOMAIN\username)
            *ptSlashInLogin = 0;
            pOpt->ldap.tADExplicitDomain = pOpt->ldap.tADLogin;
            pOpt->ldap.tADLogin = ptSlashInLogin + 1;
        }
    }

    if (pOpt->misc.bShowHelp) {
        Usage(argv[0], NULL);
    }
}

static HANDLE DumpOpenOutfileAndWriteHeader(
    _In_ LPTSTR szOutfile,
    _In_ const LPTSTR ptLdapAttributesNames[],
    _In_ DWORD dwLdapAttributesCount
    ) {
    BOOL bResult = TRUE;
    DWORD dwWritten = 0;
    HANDLE hOutfile = INVALID_HANDLE_VALUE;

    hOutfile = CreateFile(szOutfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hOutfile == INVALID_HANDLE_VALUE) {
        FATAL(_T("Failed to open outfile <%s> : <%u>"), szOutfile, GetLastError());
    }

    bResult &= WriteFile(hOutfile, LDAP_ATTR_DISTINGUISHEDNAME, (DWORD)_tcslen(LDAP_ATTR_DISTINGUISHEDNAME), &dwWritten, NULL);
    for (DWORD i = 0; i < dwLdapAttributesCount; i++) {
        bResult &= WriteFile(hOutfile, _T("\t"), (DWORD)_tcslen(_T("\t")), &dwWritten, NULL);
        bResult &= WriteFile(hOutfile, ptLdapAttributesNames[i], (DWORD)_tcslen(ptLdapAttributesNames[i]), &dwWritten, NULL);
    }
    bResult &= WriteFile(hOutfile, _T("\n"), (DWORD)_tcslen(_T("\n")), &dwWritten, NULL);
    if (!bResult) {
        CloseHandle(hOutfile);
        FATAL(_T("Failed to write header to outfile <%s>"), szOutfile);
    }

    return hOutfile;
}

static LPTSTR DumpFindConnectNCForDN(
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR ptLdapDumpDN
    ) {

    //LOG(Info, "dump=<%s> dom=<%s:%p> config=<%s:%p> sch=<%s:%p>",
    //    ptLdapDumpDN,
    //    pLdapConnectInfos->ptLDAPDomainDN, StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPDomainDN),
    //    pLdapConnectInfos->ptLDAPConfigDN, StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPConfigDN),
    //    pLdapConnectInfos->ptLDAPSchemaDN, StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPSchemaDN)
    //    );

    return
        (StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPSchemaDN) != NULL) ? pLdapConnectInfos->ptLDAPSchemaDN :
        (StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPConfigDN) != NULL) ? pLdapConnectInfos->ptLDAPConfigDN :
        (StrStr(ptLdapDumpDN, pLdapConnectInfos->ptLDAPDomainDN) != NULL) ? pLdapConnectInfos->ptLDAPDomainDN :
        NULL;
}

static DWORD DumpLdapSearchToTsv(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR ptLdapDumpDN,
    _In_ LPTSTR ptLdapFilter,
    _In_opt_ PLDAPControl *pLdapServerControls,
    _In_ const LPTSTR ptLdapAttributesNames[],
    _In_ const LDAP_DUMP_ATTR_FORMAT ptLdapAttributesFormats[],
    _In_ DWORD dwLdapAttributesCount,
    _In_ HANDLE hOutfile
    ) {
    BOOL bResult = FALSE;
    DWORD dwCount = 0;
    DWORD dwWritten = 0;
    DWORD dwLen = 0;
    LDAP_FULL_RESULT sLdapResults = { 0 };
    PTCHAR ptLdapConnectNC = NULL;

    ptLdapConnectNC = DumpFindConnectNCForDN(pLdapConnectInfos, ptLdapDumpDN);
    if (!ptLdapConnectNC) {
        FATAL(_T("Failed to find NC to connect to for DN <%s>"), ptLdapDumpDN);
    }

    // Ldap Bind operation
    bResult = LdapBind(pLdapConnectInfos, ptLdapConnectNC, pLdapOptions->tADLogin, pLdapOptions->tADPassword, pLdapOptions->tADExplicitDomain);
    if (!bResult) {
        FATAL(_T("Failed to bind to ldap server"));
    }

    // Ldap search (can be *really* memory-greedy ...)
    RtlZeroMemory(&sLdapResults, sizeof(LDAP_FULL_RESULT));
    bResult = LdapPagedSearchFullResult(pLdapOptions, pLdapConnectInfos, ptLdapConnectNC, ptLdapDumpDN, ptLdapFilter, pLdapServerControls, ptLdapAttributesNames, dwLdapAttributesCount, &sLdapResults);
    if (!bResult) {
        FATAL(_T("Failed to request <%s> on <%s>"), ptLdapFilter, ptLdapDumpDN);
    }

    for (DWORD i = 0; i < sLdapResults.dwResultCount; i++) {
        if (sLdapResults.pResults[i]->dwAttrsCount != dwLdapAttributesCount) {
            FATAL(_T("Wrong count of retreived attributes for <%s> : <%u/%u>"), sLdapResults.pResults[i]->ptDn, sLdapResults.pResults[i]->dwAttrsCount, dwLdapAttributesCount);
        }
        else {
            dwWritten = 0;
            dwLen = (DWORD)_tcslen(sLdapResults.pResults[i]->ptDn);
            bResult = WriteFile(hOutfile, sLdapResults.pResults[i]->ptDn, dwLen, &dwWritten, NULL);
            if (!bResult || dwLen != dwWritten) {
                FATAL(_T("Failed to write DN <%s> to outfile: <%u>"), sLdapResults.pResults[i]->ptDn, GetLastError());
            }

            dwWritten = 0;
            bResult = WriteFile(hOutfile, _T("\t"), 1, &dwWritten, NULL);
            if (!bResult || dwWritten != 1) {
                FATAL(_T("Failed to write whitespace after DN <%s> : <%u>"), sLdapResults.pResults[i]->ptDn, GetLastError());
            }

            for (DWORD j = 0; j < dwLdapAttributesCount; j++) {
                if (sLdapResults.pResults[i]->pLdapAttrs[j] == NULL || sLdapResults.pResults[i]->pLdapAttrs[j]->dwValuesCount == 0) {
                    bResult = TRUE;
                    dwLen = dwWritten = 0;
                }
                else {
                    DWORD dwValuesCount = sLdapResults.pResults[i]->pLdapAttrs[j]->dwValuesCount;

                    for (DWORD k = 0; k < dwValuesCount; k++) {
                        switch (ptLdapAttributesFormats[j]) {

                        case AttrStr: {
                                          dwWritten = 0;
                                          dwLen = (DWORD)_tcslen((PTCHAR)sLdapResults.pResults[i]->pLdapAttrs[j]->ppbValuesData[k]);
                                          if (dwLen != sLdapResults.pResults[i]->pLdapAttrs[j]->pdwValuesSize[k]) {
                                              FATAL(_T("Inconsistent length for str value <%u/%u> of attr <%u:%s>: <%u/%u>, <%s>"), k + 1, dwValuesCount, j, ptLdapAttributesNames[j], dwLen, sLdapResults.pResults[i]->pLdapAttrs[j]->pdwValuesSize[k], sLdapResults.pResults[i]->pLdapAttrs[j]->ppbValuesData[k]);
                                          }
                                          bResult = WriteFile(hOutfile, sLdapResults.pResults[i]->pLdapAttrs[j]->ppbValuesData[k], dwLen, &dwWritten, NULL);
                                          break;
                        }

                        case AttrHex: {
                                          PTCHAR ptHexStr = NULL;
                                          DWORD dwOutStrLen = 0;
                                          dwLen = sLdapResults.pResults[i]->pLdapAttrs[j]->pdwValuesSize[k] * 2;
                                          ptHexStr = LocalAllocCheckX(dwLen + 1);
                                          Hexify(ptHexStr, sLdapResults.pResults[i]->pLdapAttrs[j]->ppbValuesData[k], sLdapResults.pResults[i]->pLdapAttrs[j]->pdwValuesSize[k]);
                                          dwOutStrLen = (DWORD)_tcslen(ptHexStr);
                                          if (dwLen != dwOutStrLen) {
                                              FATAL(_T("Inconsistent length for hex value <%u/%u> of attr <%u:%s>: <%u/%u>"), k + 1, dwValuesCount, j, ptLdapAttributesNames[j], dwLen, sLdapResults.pResults[i]->pLdapAttrs[j]->pdwValuesSize[k]);
                                          }
                                          bResult = WriteFile(hOutfile, ptHexStr, dwLen, &dwWritten, NULL);
                                          LocalFreeCheckX(ptHexStr);
                                          break;
                        }

                        default:
                            FATAL(_T("Unknown attribute format <%u> for attribute <%u:%s> on filter <%s>"), ptLdapAttributesFormats[j], j, ptLdapAttributesNames[j], ptLdapFilter);
                        }

                        if (!bResult || dwLen != dwWritten) {
                            FATAL(_T("Failed to write value <%u/%u> of attr <%u:%s> to outfile (using format <%u>): <%u>"), k + 1, dwValuesCount, ptLdapAttributesNames[j], j, ptLdapAttributesFormats[j], GetLastError());
                        }

                        if (k < dwValuesCount - 1) {
                            dwWritten = 0;
                            bResult = WriteFile(hOutfile, _T(","), 1, &dwWritten, NULL);
                            if (!bResult || dwWritten != 1) {
                                FATAL(_T("Failed to write separator after value <%u/%u> of attr <%u:%s> : <%u>"), k + 1, dwValuesCount, j, ptLdapAttributesNames[j], GetLastError());
                            }
                        }
                    }
                }
                dwWritten = 0;
                bResult = WriteFile(hOutfile, j == dwLdapAttributesCount - 1 ? _T("\n") : _T("\t"), 1, &dwWritten, NULL);
                if (!bResult || dwWritten != 1) {
                    FATAL(_T("Failed to write whitespace after attribute <%u:%s> : <%u>"), j, ptLdapAttributesNames[j], GetLastError());
                }
            }
        }
    }

    dwCount = sLdapResults.dwResultCount;
    LdapDestroyResults(&sLdapResults);
    return dwCount;
}

static DWORD DumpAce(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR szOutfile
    ) {
    // This is the "BER" encoding of "DACL_SECURITY_INFORMATION"
    static CHAR caBerDaclSecurityInformation[] = { 0x30, 0x84, 0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x04 };
    LDAPControl sLdapControlSdFlags = { 0 };
    PLDAPControl pLdapControls[2] = { NULL };

    pLdapControls[0] = &sLdapControlSdFlags;
    pLdapControls[1] = NULL;
    sLdapControlSdFlags.ldctl_oid = LDAP_SERVER_SD_FLAGS_OID;
    sLdapControlSdFlags.ldctl_value.bv_val = caBerDaclSecurityInformation;
    sLdapControlSdFlags.ldctl_value.bv_len = sizeof(caBerDaclSecurityInformation);
    sLdapControlSdFlags.ldctl_iscritical = TRUE;

    DWORD dwCount = 0;
    HANDLE hOutfile = DumpOpenOutfileAndWriteHeader(szOutfile, gsc_aszAttrNamesAce, LDAP_DUMP_ATTR_COUNT_ACE);

    if (pLdapConnectInfos->ptLDAPRestrictedBindingDN) {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPRestrictedBindingDN, LDAP_DUMP_FILTER_NTSD, pLdapControls, gsc_aszAttrNamesAce, gsc_aeAttrFormatsAce, LDAP_DUMP_ATTR_COUNT_ACE, hOutfile);
    }
    else {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPDomainDN, LDAP_DUMP_FILTER_NTSD, pLdapControls, gsc_aszAttrNamesAce, gsc_aeAttrFormatsAce, LDAP_DUMP_ATTR_COUNT_ACE, hOutfile);
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPConfigDN, LDAP_DUMP_FILTER_NTSD, pLdapControls, gsc_aszAttrNamesAce, gsc_aeAttrFormatsAce, LDAP_DUMP_ATTR_COUNT_ACE, hOutfile);
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPSchemaDN, LDAP_DUMP_FILTER_NTSD, pLdapControls, gsc_aszAttrNamesAce, gsc_aeAttrFormatsAce, LDAP_DUMP_ATTR_COUNT_ACE, hOutfile);
    }

    CloseHandle(hOutfile);
    return dwCount;
}

static DWORD DumpObjectList(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR szOutfile
    ) {
    DWORD dwCount = 0;
    HANDLE hOutfile = DumpOpenOutfileAndWriteHeader(szOutfile, gsc_aszAttrNamesObj, LDAP_DUMP_ATTR_COUNT_OBJ);

    if (pLdapConnectInfos->ptLDAPRestrictedBindingDN) {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPRestrictedBindingDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesObj, gsc_aeAttrFormatsObj, LDAP_DUMP_ATTR_COUNT_OBJ, hOutfile);
    }
    else {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPDomainDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesObj, gsc_aeAttrFormatsObj, LDAP_DUMP_ATTR_COUNT_OBJ, hOutfile);
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPConfigDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesObj, gsc_aeAttrFormatsObj, LDAP_DUMP_ATTR_COUNT_OBJ, hOutfile);
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPSchemaDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesObj, gsc_aeAttrFormatsObj, LDAP_DUMP_ATTR_COUNT_OBJ, hOutfile);
    }

    CloseHandle(hOutfile);
    return dwCount;
}

static DWORD DumpSchema(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR szOutfile
    ) {
    DWORD dwCount = 0;
    HANDLE hOutfile = DumpOpenOutfileAndWriteHeader(szOutfile, gsc_aszAttrNamesSch, LDAP_DUMP_ATTR_COUNT_SCH);

    if (pLdapConnectInfos->ptLDAPRestrictedBindingDN) {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPRestrictedBindingDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesSch, gsc_aeAttrFormatsSch, LDAP_DUMP_ATTR_COUNT_SCH, hOutfile);
    }
    else {
        dwCount += DumpLdapSearchToTsv(pLdapOptions, pLdapConnectInfos, pLdapConnectInfos->ptLDAPSchemaDN, LDAP_DUMP_FILTER_ALL_OBJECTS, NULL, gsc_aszAttrNamesSch, gsc_aeAttrFormatsSch, LDAP_DUMP_ATTR_COUNT_SCH, hOutfile);
    }

    CloseHandle(hOutfile);
    return dwCount;
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    //
    // Variables
    //
    BOOL bResult = FALSE;
    DWORD dwCount = 0;
    LDAP_DUMP_OPTIONS sOptions = { 0 };
    LDAP_CONNECT_INFOS sInfos = { 0 };

    //
    // Options parsing & verification
    //
    if (argc == 1) {
        Usage(argv[0], NULL);
    }

    ParseOptions(&sOptions, argc, argv);

    LOG(Succ, _T("Start"));

    SetLogLevel(sOptions.misc.szLogLevel);
    if (sOptions.misc.szLogFile) {
        SetLogFile(sOptions.misc.szLogFile);
    }

    if (!sOptions.ldap.tLDAPServer) {
        Usage(argv[0], _T("Missing LDAP server"));
    }

    if ((sOptions.ldap.tADLogin && !sOptions.ldap.tADPassword) ||
        (!sOptions.ldap.tADLogin && sOptions.ldap.tADPassword)) {
        Usage(argv[0], _T("You must specify a username AND a password to use explicit authentication"));
    }

    LOG(Info, SUB_LOG("LDAP server <%s:%u>"), sOptions.ldap.tLDAPServer, sOptions.ldap.dwLDAPPort);
    if (sOptions.ldap.tADLogin) {
        LOG(Info, SUB_LOG("LDAP explicit authentication with username <%s%s%s>"),
            sOptions.ldap.tADExplicitDomain ? sOptions.ldap.tADExplicitDomain : "",
            sOptions.ldap.tADExplicitDomain ? "\\" : "",
            sOptions.ldap.tADLogin);
    }
    else {
        TCHAR szUserName[MAX_LINE] = { 0 };
        DWORD dwSize = MAX_LINE;
        GetUserName(szUserName, &dwSize);
        LOG(Info, SUB_LOG("LDAP implicit authentication with username <%s>"), szUserName);
    }
    LOG(Info, SUB_LOG("Dumping <ace:%s> <obj:%s> <sch:%s>"),
        sOptions.actions.bDumpAce ? sOptions.outfiles.szOutfileAce : _T("no"),
        sOptions.actions.bDumpObjectList ? sOptions.outfiles.szOutfileObjectList : _T("no"),
        sOptions.actions.bDumpSchema ? sOptions.outfiles.szOutfileSchema : _T("no")
        );

    if (sOptions.misc.szRestrictDumpDn) {
        LOG(Info, SUB_LOG("Dumping data on restricted DN <%s>"), sOptions.misc.szRestrictDumpDn);
        sInfos.ptLDAPRestrictedBindingDN = sOptions.misc.szRestrictDumpDn;
    }
    else {
        LOG(Info, SUB_LOG("Dumping data on default NC (domain/config/schema)"));
        sInfos.ptLDAPRestrictedBindingDN = NULL;
    }


    //
    // Getting LDAP context
    //
    LOG(Succ, _T("Connecting to LDAP server..."));
    bResult = LdapInit(&sOptions.ldap, &sInfos);
    if (!bResult) {
        FATAL(_T("Failed to connect to LDAP server"));
    }

    bResult = ExtractDomainNamingContexts(&sOptions.ldap, &sInfos);
    if (!bResult) {
        FATAL(_T("Failed to retreive LDAP context"));
    }

    LOG(Info, SUB_LOG(_T("Domain NC: <%s>")), sInfos.ptLDAPDomainDN);
    LOG(Info, SUB_LOG(_T("Config NC: <%s>")), sInfos.ptLDAPConfigDN);
    LOG(Info, SUB_LOG(_T("Schema NC: <%s>")), sInfos.ptLDAPSchemaDN);


    //
    // Dump information
    //
    if (sOptions.actions.bDumpAce) {
        LOG(Succ, _T("Dumping ACEs into <%s>"), sOptions.outfiles.szOutfileAce);
        dwCount = DumpAce(&sOptions.ldap, &sInfos, sOptions.outfiles.szOutfileAce);
        LOG(Info, SUB_LOG(_T("Count: <%u>")), dwCount);
    }

    if (sOptions.actions.bDumpObjectList) {
        LOG(Succ, _T("Dumping Object list into <%s>"), sOptions.outfiles.szOutfileObjectList);
        dwCount = DumpObjectList(&sOptions.ldap, &sInfos, sOptions.outfiles.szOutfileObjectList);
        LOG(Info, SUB_LOG(_T("Count: <%u>")), dwCount);
    }

    if (sOptions.actions.bDumpSchema) {
        LOG(Succ, _T("Dumping Schema into <%s>"), sOptions.outfiles.szOutfileSchema);
        dwCount = DumpSchema(&sOptions.ldap, &sInfos, sOptions.outfiles.szOutfileSchema);
        LOG(Info, SUB_LOG(_T("Count: <%u>")), dwCount);
    }


    //
    // Cleanup & exit
    //
    LdapDisconnectAndDestroy(&sInfos);
    LOG(Succ, _T("Done"));
    return EXIT_SUCCESS;
}
