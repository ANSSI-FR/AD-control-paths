/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_MEMBER_DEFAULT_OUTFILE  _T("control.ad.groupmember.tsv")
#define CONTROL_MEMBER_KEYWORD          _T("GROUP_MEMBER")
#define LDAP_FILTER_MEMBER              _T("(") ## NONE(LDAP_ATTR_MEMBER) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackGroupMember(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    BOOL bResult = FALSE;
    for (DWORD i = 0; i < pLdapRetreivedData->dwElementCount; i++) {
        LOG(Dbg, SUB_LOG(_T("- %u: %s")), i, (LPTSTR)pLdapRetreivedData->ppbData[i]);
        bResult = ControlWriteOutline(hOutfile, (LPTSTR)pLdapRetreivedData->ppbData[i], pLdapRetreivedData->tDN, CONTROL_MEMBER_KEYWORD);
        if (!bResult) {
            LOG(Err, _T("Cannot write outline for <%s>"), pLdapRetreivedData->tDN);
            return;
        }
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    ControlMainForeachLdapResult(argc, argv, CONTROL_MEMBER_DEFAULT_OUTFILE, LDAP_FILTER_MEMBER, LDAP_ATTR_MEMBER, NULL, CallbackGroupMember, GenericUsage);
    return EXIT_SUCCESS;
}
