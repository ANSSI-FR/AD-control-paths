/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Utils.h"
#include "..\Utils\Control.h"
#include "..\Utils\Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_SD_DEFAULT_OUTFILE       _T("control.ad.sd.tsv")
#define CONTROL_AD_OWNER_KEYWORD            _T("AD_OWNER")
#define CONTROL_AD_NULL_DACL_KEYWORD        _T("AD_NULL_DACL")
#define LDAP_FILTER_NTSECURITYDESCRIPTOR    _T("(") ## NONE(LDAP_ATTR_NTSD) ## _T("=*)")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PTCHAR gs_ptSidEveryone = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackSdOwner(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    ) {
    BOOL bResult = FALSE;
    BOOL bDaclPresent = FALSE;
    BOOL bDaclDefaulted = FALSE;
    PACL pDacl = { 0 };
    PSECURITY_DESCRIPTOR pSd = (PSECURITY_DESCRIPTOR)pLdapRetreivedData->ppbData[0];

    if (!IsValidSecurityDescriptor(pSd)) {
        LOG(Err, _T("Invalid security descriptor"));
        return;
    }

    bResult = ControlWriteOwnerOutline(hOutfile, pSd, pLdapRetreivedData->tDN, CONTROL_AD_OWNER_KEYWORD);
    if (!bResult) {
        LOG(Err, _T("Cannot write owner control relation for <%s>"), pLdapRetreivedData->tDN);
    }

    bResult = GetSecurityDescriptorDacl(pSd, &bDaclPresent, &pDacl, &bDaclDefaulted);
    if (bResult == FALSE) {
        LOG(Err, _T("Failed to get DACL <%u>"), GetLastError());
        return;
    }

    if (bDaclPresent == FALSE || pDacl == NULL) {
        LOG(Info, "Null or no DACL for element <%s>", pLdapRetreivedData->tDN);
        bResult = ControlWriteOutline(hOutfile, gs_ptSidEveryone, pLdapRetreivedData->tDN, CONTROL_AD_NULL_DACL_KEYWORD);
        if (bResult == FALSE) {
            LOG(Err, _T("Cannot write null-dacl control relation for <%s>"), pLdapRetreivedData->tDN);
            return;
        }
    }
}


/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int main(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    // This is the "BER" encoding of "OWNER_SECURITY_INFORMATION"
    static CHAR StaticBerOwnerSecurityInformation[] = { 0x30, 0x84, 0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x07 };
    BYTE pSidEveryone[SECURITY_MAX_SID_SIZE] = { 0 };
    DWORD dwSidSize = 0;
    BOOL bResult = FALSE;

    LDAPControl sLdapControlSdFlags = { 0 };
    PLDAPControl pLdapControls[2] = { NULL };
    pLdapControls[0] = &sLdapControlSdFlags;

    sLdapControlSdFlags.ldctl_oid = LDAP_SERVER_SD_FLAGS_OID;
    sLdapControlSdFlags.ldctl_value.bv_val = StaticBerOwnerSecurityInformation;
    sLdapControlSdFlags.ldctl_value.bv_len = sizeof(StaticBerOwnerSecurityInformation);
    sLdapControlSdFlags.ldctl_iscritical = TRUE;

    dwSidSize = sizeof(pSidEveryone);
    bResult = CreateWellKnownSid(WinWorldSid, NULL, (PSID)&pSidEveryone, &dwSidSize);
    if (bResult == FALSE) {
        LOG(Err, _T("Failed to create well-know SID <everyone>: <%u>"), GetLastError());
        return EXIT_FAILURE;
    }

    bResult = ConvertSidToStringSid((PSID)&pSidEveryone, &gs_ptSidEveryone);
    if (bResult == FALSE) {
        LOG(Err, _T("Failed to convert well-know SID <everyone> to string: <%u>"), GetLastError());
        return EXIT_FAILURE;
    }

    ControlMainForeachLdapResult(argc, argv, CONTROL_AD_SD_DEFAULT_OUTFILE, LDAP_FILTER_NTSECURITYDESCRIPTOR, LDAP_ATTR_NTSD, pLdapControls, CallbackSdOwner, GenericUsage);
    return EXIT_SUCCESS;
}
