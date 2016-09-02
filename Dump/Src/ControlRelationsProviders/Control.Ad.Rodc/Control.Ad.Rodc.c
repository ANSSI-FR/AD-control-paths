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

	// Reveal-OnDemand and deny NeverRevealGroup
	if (!STR_EMPTY(tokens[LdpListRevealOnDemand])) {
		while (StrNextToken(tokens[LdpListRevealOnDemand], _T(";"), &ctx, &cacheableDn)) {
			bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], cacheableDn, CONTROL_AD_RODC_CACHE_PWD_KEYWORD);
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
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackRodc, GenericUsage);

	return EXIT_SUCCESS;
}
