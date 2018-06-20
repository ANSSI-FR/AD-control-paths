/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_GPLINK_DEFAULT_OUTFILE  _T("control.ad.gplink.csv")
#define CONTROL_GPLINK_KEYWORD          _T("GPLINK")
#define LDAP_FILTER_GPLINK              _T("(") ## NONE(LDAP_ATTR_GPLINK) ## _T("=*)")
#define LDAP_PREFIX_GPLINK              _T("[ldap://")

#define GP_LINK_OPTIONS_DISABLED   1


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackGpLink(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	/* TODO: Works... But ugly C-str-parsing. Would be better with a regex or smthg */

	BOOL bResult = FALSE;
	LPTSTR ptGPLink = tokens[LdpListGPLink];
	LPTSTR ptGpoCtx = ptGPLink;
	LPTSTR ptGpoDn = NULL;
	LPTSTR ptGPLinkOptions = NULL;
	DWORD dwGPLinkOptions = 0;
	unsigned int dwGpoDnNbChar = 0;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (STR_EMPTY(tokens[LdpListGPLink]))
		return;

	ptGpoCtx = _tcschr(ptGpoCtx, _T('['));
	while (ptGpoCtx) {
		if (_tcsnicmp(LDAP_PREFIX_GPLINK, ptGpoCtx, _tcslen(LDAP_PREFIX_GPLINK)) != 0) {
			LOG(Warn, _T("Invalid gPLink value <%s>"), ptGPLink);
			break;
		}
		else {
			ptGpoCtx += _tcslen(LDAP_PREFIX_GPLINK);
			ptGPLinkOptions = _tcschr(ptGpoCtx, _T(';'));
			if (!ptGPLinkOptions) {
				LOG(Warn, _T("Invalid gPLink value <%s>"), ptGPLink);
				break;
			}
			else {
				ptGPLinkOptions++;
				dwGPLinkOptions = _tstoi(ptGPLinkOptions);
				dwGpoDnNbChar = (unsigned int)(ptGPLinkOptions - ptGpoCtx);
				ptGpoDn = (LPTSTR)malloc(dwGpoDnNbChar * sizeof(TCHAR));
				if (!ptGpoDn) {
					FATAL(_T("malloc failed for 'ptGpoDn' (%u)"), dwGpoDnNbChar);
				}
				_tcsncpy_s(ptGpoDn, dwGpoDnNbChar, ptGpoCtx, dwGpoDnNbChar - 2);
				LOG(Dbg, SUB_LOG(_T("- %s ; %u")), ptGpoDn, dwGPLinkOptions);
				if (dwGPLinkOptions & GP_LINK_OPTIONS_DISABLED) {
					LOG(Dbg, SUB_LOG(_T("  GPO is linked but disabled (GPLinkOptions=%u)")), dwGPLinkOptions);
				}
				else {
					bResult = ControlWriteOutline(hOutfile, ptGpoDn, tokens[LdpListDn], CONTROL_GPLINK_KEYWORD);
					if (!bResult) {
						LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
						return;
					}
				}
				free(ptGpoDn);
				ptGpoDn = NULL;
			}
		}
		ptGpoCtx = _tcschr(ptGpoCtx, _T('['));
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

	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackGpLink, GenericUsage);
	
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
