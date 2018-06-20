/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"



/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_PRIMARY_DEFAULT_OUTFILE _T("control.ad.primarygroup.csv")
#define CONTROL_PRIMARY_KEYWORD         _T("PRIMARY_GROUP")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void _Function_class_(FN_CONTROL_CALLBACK_RESULT) CallbackPrimaryGroup(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	CACHE_OBJECT_BY_RID searched = { 0 };
	PCACHE_OBJECT_BY_RID returned = NULL;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (STR_EMPTY(tokens[LdpListPrimaryGroupID]))
		return;

	searched.rid = tokens[LdpListPrimaryGroupID];

	bResult = CacheEntryLookup(
		ppCache,
		(PVOID)&searched,
		&returned
	);

	if (!returned) {
		LOG(Dbg, _T("cannot find object-by-rid entry for <%d>"), tokens[LdpListPrimaryGroupID]);
		return;
	}

	bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], returned->dn, CONTROL_PRIMARY_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
	}
	return;
}

static void CallbackBuildRidCache(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hOutfile);
	UNREFERENCED_PARAMETER(hDenyOutfile);

	BOOL bResult = FALSE;
	CACHE_OBJECT_BY_RID cacheEntry = { 0 };
	PCACHE_OBJECT_BY_RID inserted = NULL;
	BOOL newElement = FALSE;
	PSID pSid = NULL;
	UCHAR subAuthorityCount;
	DWORD rid;

	if (STR_EMPTY(tokens[LdpListObjectClass]))
		return;

	if (_tcscmp(tokens[LdpListObjectClass], _T("top;person;organizationalperson;user")) && _tcscmp(tokens[LdpListObjectClass], _T("top;group"))
		&& _tcscmp(tokens[LdpListObjectClass], _T("top;person;organizationalperson;user;computer")) && _tcscmp(tokens[LdpListObjectClass], _T("top;person;organizationalperson;user;inetorgperson")))
		return;

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

	subAuthorityCount = *GetSidSubAuthorityCount(pSid);
	rid = *GetSidSubAuthority(pSid, subAuthorityCount - 1);
	free(pSid);
	// Do not free as they will be inserted in the cache
	cacheEntry.dn = (LPTSTR)malloc((_tcslen(tokens[LdpListDn]) + 1) * sizeof(TCHAR));
	cacheEntry.rid = (LPTSTR)malloc((MAX_RID_SIZE + 1) * sizeof(TCHAR));
	if (!cacheEntry.dn || !cacheEntry.rid)
		FATAL(_T("Could not allocate dn/rid <%s>"), tokens[LdpListDn]);

	_tcscpy_s(cacheEntry.dn, _tcslen(tokens[LdpListDn]) + 1, tokens[LdpListDn]);
	_itot_s(rid, cacheEntry.rid, MAX_RID_SIZE + 1, 10);

	CacheEntryInsert(
		ppCache,
		(PVOID)&cacheEntry,
		sizeof(CACHE_OBJECT_BY_RID),
		&inserted,
		&newElement
	);

	if (!inserted) {
		LOG(Err, _T("cannot insert new object-by-rid cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-rid cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	else {
		LOG(Dbg, _T("inserted %s with int %d and str %s"), tokens[LdpListDn], rid, cacheEntry.rid);
	}

	return;
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptName = _T("RIDCACHE");
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
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompareRid
	);

	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildRidCache, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackPrimaryGroup, GenericUsage);

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
