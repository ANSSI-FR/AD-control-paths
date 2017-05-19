/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_EXCH_ROLE_ENTRY_OUTFILE       _T("control.exch.roleentry.csv")
#define CONTROL_EXCH_ROLE_ENTRY_KEYWORD     _T("EXCH_ROLE_ENTRY")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackExchRoleEntry(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hDenyOutfile);
	BOOL bResult = FALSE;
	LPTSTR RoleEntry = NULL;
	LPTSTR RoleEntryName = NULL;
	LPTSTR listMsExchRoleEntries = NULL;
	LPTSTR nextEntry = NULL;
	LPTSTR nextEntryField = NULL;

	// Do we have role entries
	if (STR_EMPTY(tokens[LdpListMsExchRoleEntries]))
		return;

	listMsExchRoleEntries = _tcsdup(tokens[LdpListMsExchRoleEntries]);
	RoleEntry = _tcstok_s(listMsExchRoleEntries, _T(";"), &nextEntry);
	while (RoleEntry) {
		RoleEntryName = _tcstok_s(RoleEntry, _T(","), &nextEntryField);
		RoleEntryName = _tcstok_s(NULL, _T(","), &nextEntryField);

		bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], RoleEntryName, CONTROL_EXCH_ROLE_ENTRY_KEYWORD);
		if (!bResult) {
			LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		}

		nextEntryField = NULL;
		RoleEntry = _tcstok_s(NULL, _T(";"), &nextEntry);
	}
	free(listMsExchRoleEntries);
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackExchRoleEntry, GenericUsage);

	return EXIT_SUCCESS;
}
