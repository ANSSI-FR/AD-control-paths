/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"



/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_SIDHIST_DEFAULT_OUTFILE _T("control.ad.sidhistory.csv")
#define CONTROL_SIDHIST_KEYWORD         _T("SID_HISTORY")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackSidHistory(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PSID pSid;
	LPTSTR listSidHistory = NULL;
	LPTSTR psSid = NULL;
	LPTSTR next = NULL;
	DWORD szsSid = 0;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;
	LPTSTR sidHistoryTrustee = NULL;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (STR_EMPTY(tokens[LdpListSIDHistory]))
		return;

	listSidHistory = _tcsdup(tokens[LdpListSIDHistory]);
	psSid = _tcstok_s(listSidHistory, _T(";"), &next);
	while (psSid) {
		szsSid = (DWORD)_tcslen(psSid);
		pSid = malloc((szsSid * sizeof(TCHAR)) / 2);
		if (!pSid)
			FATAL(_T("Cannot allocate pSidHistory for %s"), tokens[LdpListDn]);
		Unhexify(pSid, psSid);


		bResult = IsValidSid(pSid);
		if (!bResult) {
			LOG(Err, _T("Invalid SIDHistory Sid <%s> for <%s>"), psSid, tokens[LdpListDn]);
			return;
		}
		ConvertSidToStringSid(pSid, &searched.sid);
		CharLower(searched.sid);
		bResult = CacheEntryLookup(
			ppCache,
			(PVOID)&searched,
			&returned
		);
		if (!returned) {
			LOG(Dbg, _T("cannot find object-by-sid entry for <%d>"), tokens[LdpListSIDHistory]);
			sidHistoryTrustee = searched.sid;
		}
		else {
			sidHistoryTrustee = returned->dn;
		}
		bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], sidHistoryTrustee, CONTROL_SIDHIST_KEYWORD);
		if (!bResult) {
			LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		}
		LocalFree(searched.sid);
		psSid = _tcstok_s(NULL, _T(";"), &next);
		free(pSid);
	}
	free(listSidHistory);
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptName = _T("SIDCACHE");
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
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackSidHistory, GenericUsage);

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
