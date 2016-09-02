/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_RODC_DEFAULT_OUTFILE       _T("control.ad.rodc.csv")
#define CONTROL_AD_RODC_CACHE_PWD_KEYWORD     _T("RODC_CACHE_PWD")
#define CONTROL_AD_RODC_MANAGED_BY_KEYWORD    _T("RODC_MANAGED_BY")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackRodc(
	_In_ CSV_HANDLE hOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PSECURITY_DESCRIPTOR pSd;
	BOOL bOwnerDefaulted = FALSE;
	PSID pSidOwner = NULL;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;
	LPTSTR manager = NULL;

	// Only way to know we have a rodc
	if (STR_EMPTY(tokens[LdpListPrimaryGroupID]) || _tcscmp(tokens[LdpListPrimaryGroupID], _T("521")))
		return;

	// managedBy
	if (!STR_EMPTY(tokens[LdpListManagedBy])) {
		ConvertSidToStringSid(tokens[LdpListManagedBy], &searched.sid);
		CharLower(searched.sid);
		bResult = CacheEntryLookup(
			ppCache,
			(PVOID)&searched,
			&returned
		);
		if (!returned) {
			LOG(Dbg, _T("cannot find object-by-sid entry for <%d>"), searched.sid);
			manager = searched.sid;
		}
		else {
			manager = returned->dn;
		}
		bResult = ControlWriteOutline(hOutfile, manager, tokens[LdpListDn], CONTROL_AD_RODC_MANAGED_BY_KEYWORD);
		if (!bResult) {
			LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		}
		LocalFree(searched.sid);
	}

	// Reveal-OnDemand and deny NeverRevealGroup
	PTCHAR *ctx = NULL;
	PTCHAR *tok = NULL;
	if (!STR_EMPTY(tokens[LdpListRevealOnDemand])) {
		searched.sid = StrNextToken(tokens[LdpListRevealOnDemand], _T(";"), &ctx, &tok);
		bResult = CacheEntryLookup(
			ppCache,
			(PVOID)&searched,
			&returned
		);
		if (!returned) {
			LOG(Dbg, _T("cannot find object-by-sid entry for <%d>"), searched.sid);
			manager = searched.sid;
		}
		else {
			manager = returned->dn;
		}
		bResult = ControlWriteOutline(hOutfile, manager, tokens[LdpListDn], CONTROL_AD_RODC_MANAGED_BY_KEYWORD);
		if (!bResult) {
			LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		}
		LocalFree(searched.sid);
	}


}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptName = _T("SIDCACHE");
	bCacheBuilt = FALSE;
	CacheCreate(
		&ppCache,
		ptName,
		pfnCompare
	);
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildSidCache, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackRodc, GenericUsage);

	return EXIT_SUCCESS;
}
