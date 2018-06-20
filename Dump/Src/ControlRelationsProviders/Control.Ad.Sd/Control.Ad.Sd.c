/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_SD_DEFAULT_OUTFILE       _T("control.ad.sd.csv")
#define CONTROL_AD_OWNER_KEYWORD            _T("SD_OWNER")
#define CONTROL_AD_NULL_DACL_KEYWORD        _T("SD_NULL_DACL")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackSdOwner(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PSECURITY_DESCRIPTOR pSd;
	BOOL bOwnerDefaulted = FALSE;
	PSID pSidOwner = NULL;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;
	LPTSTR owner = NULL;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (STR_EMPTY(tokens[LdpAceSd]))
		return;

	pSd = malloc((_tcslen(tokens[LdpAceSd]) * sizeof(TCHAR)) / 2);
	if (!pSd)
		FATAL(_T("Cannot allocate pSd for %s"), tokens[LdpAceSd]);
	Unhexify(pSd, tokens[LdpAceSd]);
	if (!IsValidSecurityDescriptor(pSd)) {
		LOG(Err, _T("Invalid security descriptor"));
		return;
	}
	bResult = GetSecurityDescriptorOwner(pSd, &pSidOwner, &bOwnerDefaulted);
	if (!bResult) {
		LOG(Err, _T("Cannot get Owner from SD : <%u>"), GetLastError());
		return;
	}
	if (!pSidOwner) {
		LOG(Dbg, _T("No Owner from SD : <%s>"), tokens[LdpAceDn]);
		return;
	}

	ConvertSidToStringSid(pSidOwner, &searched.sid);
	// Owner is SELF
	if (!_tcsicmp(_T("S-1-5-10"), searched.sid)) {
		owner = tokens[LdpAceDn];
	}
	else {
		CharLower(searched.sid);
		bResult = CacheEntryLookup(
			ppCache,
			(PVOID)&searched,
			&returned
		);
		if (!returned) {
			LOG(Dbg, _T("cannot find object-by-sid entry for SD owner of <%s>"), tokens[LdpAceDn]);
			owner = searched.sid;
		}
		else {
			owner = returned->dn;
		}
	}
	bResult = ControlWriteOutline(hOutfile, owner, tokens[LdpAceDn], CONTROL_AD_OWNER_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpAceDn]);
	}
	LocalFree(searched.sid);
	free(pSd);
	return;
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptName = _T("SIDCACHE");
	bCacheBuilt = FALSE;
	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	CacheCreate(
		&ppCache,
		ptName,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildSidCache, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackSdOwner, GenericUsage);

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
