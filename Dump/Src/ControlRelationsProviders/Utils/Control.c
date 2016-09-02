/* --- INCLUDES ------------------------------------------------------------- */
#include "Control.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LOG_LEVEL gs_debugLevel = Succ;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void GenericUsage(
	_In_ PTCHAR  progName
) {
	LOG(Bypass, _T("Usage : %s <general options>"), progName);
	LOG(Bypass, _T("> General options :"));
	ControlUsage();

	ExitProcess(EXIT_FAILURE);
}

void ControlUsage(
    ) {
    LOG(Bypass, SUB_LOG(_T("-h/H          : show this help")));
	LOG(Bypass, SUB_LOG(_T("-I <Infile>  : infile path")));
	LOG(Bypass, SUB_LOG(_T("-A <Infile>  : addtional infile path")));
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
    optind = 1;
    opterr = 0;

    while ((curropt = getopt(argc, argv, _T("hHA:I:O:Y:L:D:"))) != -1) {
        switch (curropt) {
        case _T('O'): pControlOptions->ptOutfile = optarg; break;
		case _T('Y'): pControlOptions->ptDenyOutfile = optarg; break;
		case _T('I'): pControlOptions->ptInfile = optarg; break;
		case _T('A'): 
			if(bCacheBuilt)
				pControlOptions->ptInfile = optarg; 
			break;
        case _T('L'): pControlOptions->ptLogFile = optarg; break;
        case _T('D'): pControlOptions->ptLogLevel = optarg; break;
        case _T('h'):
        case _T('H'): pControlOptions->bShowHelp = TRUE; break;
        case _T('?'): optind++; break; // skipping unknown options
        }
    }
}


BOOL ControlWriteOutline(
    _In_ CSV_HANDLE hOutfile,
    _In_ PTCHAR ptMaster,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    ) {
	
	BOOL bResult = FALSE;
	LPTSTR csvRecord[3];
	DWORD csvRecordNumber = 3;
	csvRecord[0] = ptMaster;
	csvRecord[1] = ptSlave;
	csvRecord[2] = ptKeyword;

	bResult = CsvWriteNextRecord(hOutfile, csvRecord, &csvRecordNumber);
	if (!bResult) {
		LOG(Err, _T("Failed to write CSV outfile: <err:%#08x>"), CsvGetLastError(hOutfile));
	return FALSE;
	}

    return TRUE;
}

BOOL ControlWriteOwnerOutline(
	_In_ CSV_HANDLE hOutfile,
	_In_ PSECURITY_DESCRIPTOR pSdOwner,
	_In_ PTCHAR ptSlave,
	_In_ PTCHAR ptKeyword
) {
	BOOL bResult = FALSE;
	BOOL bOwnerDefaulted = FALSE;
	PSID pSidOwner = NULL;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;
	LPTSTR owner = NULL;

	bResult = GetSecurityDescriptorOwner(pSdOwner, &pSidOwner, &bOwnerDefaulted);
	if (!bResult) {
		LOG(Err, _T("Cannot get Owner from SD : <%u>"), GetLastError());
		return FALSE;
	}

	ConvertSidToStringSid(pSidOwner, &searched.sid);
	CharLower(searched.sid);
	bResult = CacheEntryLookup(
		ppCache,
		(PVOID)&searched,
		&returned
	);
	
	if (!returned) {
		LOG(Dbg, _T("cannot find object-by-sid entry for <%d>"), searched.sid);
		owner = searched.sid;
	}
	else {
		owner = returned->dn;
	}

	bResult = ControlWriteOutline(hOutfile, owner, ptSlave, ptKeyword);
	LocalFree(searched.sid);
	return bResult;
}


void CallbackBuildSidCache(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_In_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hOutfile);
	UNREFERENCED_PARAMETER(hDenyOutfile);

	BOOL bResult = FALSE;
	CACHE_OBJECT_BY_SID cacheEntry = { 0 };
	PCACHE_OBJECT_BY_SID inserted = NULL;
	BOOL newElement = FALSE;
	PSID pSid = NULL;

	if (STR_EMPTY(tokens[LdpListObjectSid]))
		return;

	LOG(Dbg, _T("Object <%s> has hex SID <%s>"), tokens[LdpListDn], tokens[LdpListObjectSid]);

	pSid = (PSID)malloc(SECURITY_MAX_SID_SIZE);
	if (!pSid)
		FATAL(_T("Could not allocate SID <%s>"), tokens[LdpListObjectSid]);

	if (_tcslen(tokens[LdpListObjectSid]) / 2 > SECURITY_MAX_SID_SIZE) {
		FATAL(_T("Hex sid <%s> too long"), tokens[LdpListObjectSid]);
	}
	Unhexify(pSid, tokens[LdpListObjectSid]);
	bResult = IsValidSid(pSid);
	if (!bResult) {
		FATAL(_T("Invalid SID <%s> for <%s>"), tokens[LdpListObjectSid], tokens[LdpListDn]);
	}

	ConvertSidToStringSid(pSid, &cacheEntry.sid);
	free(pSid);
	cacheEntry.dn = malloc((_tcslen(tokens[LdpListDn]) + 1) * sizeof(TCHAR));
	if (!cacheEntry.dn)
		FATAL(_T("Could not allocate dc <%s>"), tokens[LdpListDn]);
	_tcscpy_s(cacheEntry.dn, _tcslen(tokens[LdpListDn]) + 1, tokens[LdpListDn]);

	CacheEntryInsert(
		ppCache,
		(PVOID)&cacheEntry,
		sizeof(CACHE_OBJECT_BY_SID),
		&inserted,
		&newElement
	);

	if (!inserted) {
		LOG(Err, _T("cannot insert new object-by-sid cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-sid cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	return;
}



void ControlMainForeachCsvResult(
	_In_ int argc,
	_In_ PTCHAR argv[],
	_In_ PTCHAR ptOutfileHeader[],
	_In_ FN_CONTROL_CALLBACK_RESULT pfnCallback,
	_In_ FN_USAGE_CALLBACK pfnUsage
) {

	/* Variables */
	BOOL bResult = FALSE;
	CSV_HANDLE hOutfile = CSV_INVALID_HANDLE_VALUE;
	CSV_HANDLE hDenyOutfile = CSV_INVALID_HANDLE_VALUE;
	CSV_HANDLE hInfile = CSV_INVALID_HANDLE_VALUE;
	CONTROL_OPTIONS sControlOptions = { 0 };
	PTCHAR *pptAttrsListForCsv = ptOutfileHeader;
	LPTSTR *tokens = { 0 };
	DWORD headerCount;
	DWORD i = 0;

	LPTSTR pFilename;
	LPTSTR next = NULL;
	LPTSTR listFilename;

	/* Options parsing */
	RtlZeroMemory(&sControlOptions, sizeof(CONTROL_OPTIONS));
	ControlParseOptions(&sControlOptions, argc, argv);

	if (sControlOptions.ptLogFile)  LogSetLogFile(sControlOptions.ptLogFile);
	if (sControlOptions.ptLogLevel) LogSetLogLevel(LOG_ALL_TYPES, sControlOptions.ptLogLevel);
	if (sControlOptions.bShowHelp) {
		pfnUsage(argv[0]);
	}

	LOG(Succ, _T("Starting"));

	/* Outfiles opening */
	if (!sControlOptions.ptOutfile) {
		FATAL(_T("No outfile specified. Please use -O."));
	}

	LOG(Info, _T("Opening outfile <%s>"), sControlOptions.ptOutfile);
	if (!bCacheBuilt) {
		bResult = CsvOpenWrite(sControlOptions.ptOutfile, OUTFILE_TOKEN_COUNT, pptAttrsListForCsv, &hOutfile);
		if (!bResult) {
			FATAL(_T("Failed to open CSV outfile <%s>: <err:%#08x>"), sControlOptions.ptOutfile, CsvGetLastError(hOutfile));
		}
	}
	else {
		bResult = CsvOpenAppend(sControlOptions.ptOutfile, 0, NULL, &hOutfile);
		if (!bResult) {
			FATAL(_T("Failed to open CSV outfile <%s>: <err:%#08x>"), sControlOptions.ptOutfile, CsvGetLastError(hOutfile));
		}
	}

	if (sControlOptions.ptDenyOutfile) {
		bResult = CsvOpenWrite(sControlOptions.ptDenyOutfile, OUTFILE_TOKEN_COUNT, pptAttrsListForCsv, &hDenyOutfile);
		if (!bResult) {
			FATAL(_T("Failed to open CSV Deny outfile <%s>: <err:%#08x>"), sControlOptions.ptDenyOutfile, CsvGetLastError(hDenyOutfile));
		}
	}


	/* Infiles opening */
	LOG(Info, _T("Opening infile(s) <%s>"), sControlOptions.ptInfile);
	listFilename = _tcsdup(sControlOptions.ptInfile);
	pFilename = _tcstok_s(listFilename, _T(","), &next);
	while (pFilename) {
		bResult = CsvOpenRead(pFilename, &headerCount, NULL, &hInfile);
		if (!bResult) {
			FATAL(_T("Failed to open CSV Infile <%s>: <err:%#08x>"), sControlOptions.ptInfile, CsvGetLastError(hInfile));
		}
		while (CsvGetNextRecord(hInfile, &tokens, &gs_recordNumber)) {
			if (!tokens)
				FATAL(_T("Failed to retrieve CSV record from %s"), sControlOptions.ptInfile);
			LOG(Dbg, SUB_LOG(_T("%s :")), tokens[LdpListDn]);
			// Convert records to lowercase as AD is case insensitive and there are inconsistencies, i.e. gplinks
			for (i = 0; i < headerCount; i++) {
				if (!STR_EMPTY(tokens[i]))
					CharLower(tokens[i]);
			}
			pfnCallback(hOutfile, hDenyOutfile, tokens);
			CsvRecordArrayHeapFree(tokens, headerCount);
		}
		CsvClose(&hInfile);
		pFilename = _tcstok_s(NULL, _T(","), &next);

	}
	free(listFilename);
	LOG(Succ, _T("Input file processed."));
	
	/* Done */
	CsvClose(&hOutfile);
	if (sControlOptions.ptDenyOutfile) {
		CsvClose(&hDenyOutfile);
	}
}

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompare(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
) {
	UNREFERENCED_PARAMETER(Table);
	return CacheCompareStr(((PCACHE_OBJECT_BY_SID)(FirstStruct))->sid, ((PCACHE_OBJECT_BY_SID)(SecondStruct))->sid);
}

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompareRid(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
) {
	UNREFERENCED_PARAMETER(Table);
	return CacheCompareStr(((PCACHE_OBJECT_BY_RID)(FirstStruct))->rid, ((PCACHE_OBJECT_BY_RID)(SecondStruct))->rid);
}

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompareDn(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
) {
	UNREFERENCED_PARAMETER(Table);
	return CacheCompareStr(((PCACHE_OBJECT_BY_DN)(FirstStruct))->dn, ((PCACHE_OBJECT_BY_DN)(SecondStruct))->dn);
}
