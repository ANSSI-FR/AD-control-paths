/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_GPLINK_DEFAULT_OUTFILE  _T("control.ad.gplink.tsv")
#define CONTROL_GPLINK_KEYWORD          _T("GPLINK")
#define LDAP_FILTER_GPLINK              _T("(") ## NONE(LDAP_ATTR_GPLINK) ## _T("=*)")
#define LDAP_PREFIX_GPLINK              _T("[ldap://")

#define GP_LINK_OPTIONS_DISABLED   1


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackGpLink(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    /* TODO: Works... But ugly C-str-parsing. Would be better with a regex or smthg */

    BOOL bResult = FALSE;
    LPTSTR ptGPLink = (LPTSTR)pLdapRetreivedData->ppbData[0];
    LPTSTR ptGpoCtx = ptGPLink, ptGpoDn = NULL, ptGPLinkOptions = NULL;
    DWORD dwGPLinkOptions = 0, dwGpoDnSize = 0;

    while (ptGpoCtx = _tcschr(ptGpoCtx, _T('['))) {
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
                dwGpoDnSize = (DWORD)(ptGPLinkOptions - ptGpoCtx);
                ptGpoDn = malloc(dwGpoDnSize);
                if (!ptGpoDn) {
                    FATAL(_T("malloc failed for 'ptGpoDn' (%u)"), dwGpoDnSize);
                }
                _tcsncpy_s(ptGpoDn, dwGpoDnSize, ptGpoCtx, dwGpoDnSize - 1);
                LOG(Dbg, SUB_LOG(_T("- %s ; %u")), ptGpoDn, dwGPLinkOptions);
                if (dwGPLinkOptions & GP_LINK_OPTIONS_DISABLED) {
                    LOG(Dbg, SUB_LOG(_T("  GPO is linked but disabled (GPLinkOptions=%u)")), dwGPLinkOptions);
                }
                else {
                    bResult = ControlWriteOutline(hOutfile, ptGpoDn, pLdapRetreivedData->tDN, CONTROL_GPLINK_KEYWORD);
                    if (!bResult) {
                        LOG(Err, _T("Cannot write outline for <%s>"), pLdapRetreivedData->tDN);
                        return;
                    }
                }
                free(ptGpoDn);
            }
        }
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    ControlMainForeachLdapResult(argc, argv, CONTROL_GPLINK_DEFAULT_OUTFILE, LDAP_FILTER_GPLINK, LDAP_ATTR_GPLINK, NULL, CallbackGpLink, GenericUsage);
    return EXIT_SUCCESS;
}
