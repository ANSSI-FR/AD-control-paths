/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_MEMBER_DEFAULT_OUTFILE  _T("control.ad.groupmember.csv")
#define CONTROL_MEMBER_KEYWORD          _T("GROUP_MEMBER")
#define LDAP_FILTER_MEMBER              _T("(") ## NONE(LDAP_ATTR_MEMBER) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackGroupMember(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	LPTSTR pMember = NULL;
	LPTSTR next = NULL;
	LPTSTR listMember = NULL;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (STR_EMPTY(tokens[LdpListMember]))
		return;

		listMember = _tcsdup(tokens[LdpListMember]);
		pMember = _tcstok_s(listMember, _T(";"), &next);
		while (pMember) {
			bResult = ControlWriteOutline(hOutfile, pMember, tokens[LdpListDn], CONTROL_MEMBER_KEYWORD);
			if (!bResult) {
				LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
			}
			pMember = _tcstok_s(NULL, _T(";"), &next);
		}
		free(listMember);
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

	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackGroupMember, GenericUsage);

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
