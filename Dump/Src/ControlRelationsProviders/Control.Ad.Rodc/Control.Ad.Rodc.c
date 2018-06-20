/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_RODC_DEFAULT_OUTFILE       _T("control.ad.rodc.csv")
#define CONTROL_AD_RODC_CACHE_PWD_KEYWORD     _T("RODC_CACHE_PWD")
#define CONTROL_AD_RODC_MANAGED_BY_KEYWORD    _T("RODC_MANAGED_BY")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackRodc(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	LPTSTR ctx = NULL;
	LPTSTR cacheableDn = NULL;

	// Only way to know we have a rodc
	if (STR_EMPTY(tokens[LdpListPrimaryGroupID]) || _tcscmp(tokens[LdpListPrimaryGroupID], _T("521")))
		return;

	// managedBy
	if (!STR_EMPTY(tokens[LdpListManagedBy])) {
		bResult = ControlWriteOutline(hOutfile, tokens[LdpListManagedBy], tokens[LdpListDn], CONTROL_AD_RODC_MANAGED_BY_KEYWORD);
		if (!bResult) {
			LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		}
	}

	// Reveal-OnDemand
	if (!STR_EMPTY(tokens[LdpListRevealOnDemand])) {
		while (StrNextToken(tokens[LdpListRevealOnDemand], _T(";"), &ctx, &cacheableDn)) {
			bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], cacheableDn, CONTROL_AD_RODC_CACHE_PWD_KEYWORD);
			if (!bResult) {
				LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
			}
		}
	}
	// Deny NeverRevealGroup
	ctx = NULL;
	if (!STR_EMPTY(tokens[LdpListNeverReveal])) {
		while (StrNextToken(tokens[LdpListNeverReveal], _T(";"), &ctx, &cacheableDn)) {
			bResult = ControlWriteOutline(hDenyOutfile, tokens[LdpListDn], cacheableDn, CONTROL_AD_RODC_CACHE_PWD_KEYWORD);
			if (!bResult) {
				LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
			}
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

	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackRodc, GenericUsage);

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
