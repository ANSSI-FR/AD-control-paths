/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_EXCH_ROLE_OUTFILE       _T("control.exch.role.csv")
#define CONTROL_EXCH_ROLE_KEYWORD     _T("EXCH_ROLE")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackExchRole(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hDenyOutfile);
	BOOL bResult = FALSE;

	// Do we have msExchRoleAssignment class
	if (STR_EMPTY(tokens[LdpListMsExchUserLink]) || STR_EMPTY(tokens[LdpListMsExchRoleLink]))
		return;
	
	bResult = ControlWriteOutline(hOutfile, tokens[LdpListMsExchUserLink], tokens[LdpListMsExchRoleLink], CONTROL_EXCH_ROLE_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
	}
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackExchRole, GenericUsage);

	return EXIT_SUCCESS;
}
