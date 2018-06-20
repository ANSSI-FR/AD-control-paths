/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_CONTAINER_DEFAULT_OUTFILE   _T("control.ad.container.csv")
#define CONTROL_CONTAINER_KEYWORD           _T("CONTAINER_HIERARCHY")


/* --- TYPES -------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PTCHAR gs_domainNC = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
//
// Container hierarchy control is given through 2 mechanisms: ACE inheritance (for OUs and containers)
// and GPO inheritance (for OUs only).
// ACE inheritance is blocked by sdprop for objects with admincount==1, but control is retained through GPO if it's an OU
// GPO inheritance is not really blockable as it can be overridden (!= enforced), and its control is transitive along sub containers
// So if admincount==1, we go upwards to the 1st OU object to give control.
//
static void CallbackContainerHierarchy(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PTCHAR *ppExplodedDn = NULL;
	PTCHAR ptDnParent = NULL;
	size_t len = 0;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (gs_recordNumber == 1)
		gs_domainNC = _tcsdup(tokens[0]);

	static const PTCHAR ptDnStop[] = {
		_T("cn=configuration,"),
		_T("cn=schema,cn=configuration,"),
	};

	if (_tcscmp(tokens[LdpListDn], gs_domainNC) == 0) {
		LOG(Dbg, SUB_LOG(_T("- skip (match <%s>)")), gs_domainNC);
		return;
	}

	for (DWORD i = 0; i < ARRAY_COUNT(ptDnStop); i++) {
		len = _tcslen(ptDnStop[i]);
		if (_tcsncmp(tokens[LdpListDn], ptDnStop[i], len) == 0 &&
			_tcscmp(tokens[LdpListDn] + len, gs_domainNC) == 0) {
			LOG(Dbg, SUB_LOG(_T("- skip (match <%s%s>)")), ptDnStop[i], gs_domainNC);
			return;
		}
	}

	ppExplodedDn = ldap_explode_dn(tokens[LdpListDn], FALSE);
	if (!ppExplodedDn) {
		LOG(Err, _T("Cannot explode DN <%s>"), tokens[LdpListDn]);
		return;
	}
	if (!ppExplodedDn[0]) {
		LOG(Err, _T("Exploded DN <%s> does not even have 1 element"), tokens[LdpListDn]);
		return;
	}
	ptDnParent = tokens[LdpListDn] + _tcslen(ppExplodedDn[0]) + 1;
	if (!STR_EMPTY(tokens[LdpListAdminCount]) && !_tcscmp(tokens[LdpListAdminCount], _T("1"))) {
		for (DWORD i = 1; _tcsncmp(ptDnParent, _T("cn"), 2) == 0; i++) {
			ptDnParent += _tcslen(ppExplodedDn[i]) + 1;
		}
	}

	LOG(Dbg, SUB_LOG(_T("- %s")), ptDnParent);
	bResult = ControlWriteOutline(hOutfile, ptDnParent, tokens[LdpListDn], CONTROL_CONTAINER_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
		return;
	}
	ldap_value_free(ppExplodedDn);
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

	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackContainerHierarchy, GenericUsage);

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
