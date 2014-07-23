/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_PRIMARY_DEFAULT_OUTFILE _T("control.ad.primarygroup.tsv")
#define CONTROL_PRIMARY_KEYWORD         _T("PRIMARY_GROUP")
#define LDAP_FILTER_PRIMARY             _T("(") ## NONE(LDAP_ATTR_PRIMARYGROUPID) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackPrimaryGroup(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    BOOL bResult = FALSE;
    DWORD dwPrimaryGroupRid = 0;
    DWORD dwDataSize = 0;
    PTCHAR ptDnPrimaryGroup = NULL;

    for (DWORD i = 0; i < pLdapRetreivedData->dwElementCount; i++) {
        dwDataSize = pLdapRetreivedData->pdwDataSize[i];
        for (DWORD j = 0; j < dwDataSize; j++) {
            dwPrimaryGroupRid += ((pLdapRetreivedData->ppbData[i][j] & 0x0F) << (((dwDataSize - 1) - j) * 4)); // wtf
        }
        LOG(Dbg, SUB_LOG(_T("- %u: %x")), i, dwPrimaryGroupRid);

        bResult = LdapResolveRid(dwPrimaryGroupRid, &ptDnPrimaryGroup);
        if (!bResult) {
            LOG(Warn, _T("Cannot resolve primary group RID <%x>"), dwPrimaryGroupRid);
            // Do not return here, since unresolvable domain SID could exist
        }
        LOG(Dbg, SUB_LOG(_T("- %u: %s")), i, ptDnPrimaryGroup);

        bResult = ControlWriteOutline(hOutfile, pLdapRetreivedData->tDN, ptDnPrimaryGroup, CONTROL_PRIMARY_KEYWORD);
        if (!bResult) {
            LOG(Err, _T("Cannot write outline for <%s>"), pLdapRetreivedData->tDN);
        }

        free(ptDnPrimaryGroup);
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    ControlMainForeachLdapResult(argc, argv, CONTROL_PRIMARY_DEFAULT_OUTFILE, LDAP_FILTER_PRIMARY, LDAP_ATTR_PRIMARYGROUPID, NULL, CallbackPrimaryGroup, GenericUsage);
    return EXIT_SUCCESS;
}
