/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_SIDHIST_DEFAULT_OUTFILE _T("control.ad.sidhistory.tsv")
#define CONTROL_SIDHIST_KEYWORD         _T("SID_HISTORY")
#define LDAP_FILTER_SIDHIST             _T("(") ## NONE(LDAP_ATTR_SIDHISTORY) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackSidHistory(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    BOOL bResult = FALSE;
    PTCHAR ptDnSidHistory = NULL;

    for (DWORD i = 0; i < pLdapRetreivedData->dwElementCount; i++) {
        bResult = LdapResolveRawSid((PSID)pLdapRetreivedData->ppbData[i], &ptDnSidHistory);
        if (!bResult) {
            LOG(Warn, SUB_LOG(_T("%u: invalid sid history")), i);
            continue;
        }

        bResult = ControlWriteOutline(hOutfile, pLdapRetreivedData->tDN, ptDnSidHistory, CONTROL_SIDHIST_KEYWORD);
        if (!bResult) {
            LOG(Err, _T("Cannot write outline for <%s>"), pLdapRetreivedData->tDN);
        }
        free(ptDnSidHistory);
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    ControlMainForeachLdapResult(argc, argv, CONTROL_SIDHIST_DEFAULT_OUTFILE, LDAP_FILTER_SIDHIST, LDAP_ATTR_SIDHISTORY, NULL, CallbackSidHistory, GenericUsage);
    return EXIT_SUCCESS;
}
