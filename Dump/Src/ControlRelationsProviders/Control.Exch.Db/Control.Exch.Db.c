/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_EXCH_DB_DEFAULT_OUTFILE       _T("control.exch.db.csv")
#define CONTROL_EXCH_DB_CONTAINER_KEYWORD     _T("EXCH_DB_CONTAINER")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackExchDb(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hDenyOutfile);
	BOOL bResult = FALSE;

	// Do we have a mailbox and a containing DB
	if (STR_EMPTY(tokens[LdpListHomeMDB]) || STR_EMPTY(tokens[LdpListMail]))
		return;

	bResult = ControlWriteOutline(hOutfile, tokens[LdpListHomeMDB], tokens[LdpListMail], CONTROL_EXCH_DB_CONTAINER_KEYWORD);
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
	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackExchDb, GenericUsage);

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
