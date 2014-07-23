/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_CONTAINER_DEFAULT_OUTFILE   _T("control.ad.container.tsv")
#define CONTROL_CONTAINER_KEYWORD           _T("CONTAINER_HIERARCHY")
#define LDAP_FILTER_ALL_OBJECTS             _T("(") ## NONE(LDAP_ATTR_OBJECT_CLASS) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackContainerHierarchy(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    BOOL bResult = FALSE;
    PCHAR* ppExplodedDn = NULL;
    PTCHAR ptDnParent = NULL;
    PTCHAR ptDomainNc = LdapGetDomainNC();
    size_t len = 0;

    static const PTCHAR ptDnStop[] = {
        _T("CN=Configuration,"),
        _T("CN=Schema,CN=Configuration,"),
    };

    if (_tcscmp(pLdapRetreivedData->tDN, ptDomainNc) == 0) {
        LOG(Dbg, SUB_LOG(_T("- skip (match <%s>)")), ptDomainNc);
        return;
    }

    for (DWORD i = 0; i < ARRAY_COUNT(ptDnStop); i++) {
        len = _tcslen(ptDnStop[i]);
        if (_tcsncmp(pLdapRetreivedData->tDN, ptDnStop[i], len) == 0 &&
            _tcscmp(pLdapRetreivedData->tDN + len, ptDomainNc) == 0) {
            LOG(Dbg, SUB_LOG(_T("- skip (match <%s%s>)")), ptDnStop[i], ptDomainNc);
            return;
        }
    }

    ppExplodedDn = ldap_explode_dn(pLdapRetreivedData->tDN, FALSE);
    if (!ppExplodedDn) {
        LOG(Err, _T("Cannot explode DN <%s>"), pLdapRetreivedData->tDN);
    }
    else if (!ppExplodedDn[0]) {
        LOG(Err, _T("Exploded DN <%s> does not even have 1 element"), pLdapRetreivedData->tDN);
    }
    else {
        ptDnParent = pLdapRetreivedData->tDN + _tcslen(ppExplodedDn[0]) + 1;
        LOG(Dbg, SUB_LOG(_T("- %s")), ptDnParent);
        bResult = ControlWriteOutline(hOutfile, ptDnParent, pLdapRetreivedData->tDN, CONTROL_CONTAINER_KEYWORD);
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
    ControlMainForeachLdapResult(argc, argv, CONTROL_CONTAINER_DEFAULT_OUTFILE, LDAP_FILTER_ALL_OBJECTS, LDAP_ATTR_OBJECT_CLASS, NULL, CallbackContainerHierarchy, GenericUsage);
    return EXIT_SUCCESS;
}
