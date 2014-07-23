#ifndef __LDAP_H__
#define __LDAP_H__


/* --- INCLUDES ------------------------------------------------------------- */
#include "Utils.h"
#include <Winldap.h>
#include <WinBer.h>
#include <NtLdap.h>
#include <Shlwapi.h>


/* --- DEFINES -------------------------------------------------------------- */
#define AD_LDAP_SEARCH_LIMIT        1
#define DEFAULT_LDAP_PORT           389

#define LDAP_ATTR_COMMONNAME        _T("cn")
#define LDAP_ATTR_GPLINK            _T("gPLink")
#define LDAP_ATTR_OBJECT_CLASS      _T("objectClass")
#define LDAP_ATTR_MEMBER            _T("member")
#define LDAP_ATTR_PRIMARYGROUPID    _T("primaryGroupID")
#define LDAP_ATTR_SIDHISTORY        _T("sIDHistory")
#define LDAP_ATTR_NTSD              _T("nTSecurityDescriptor")
#define LDAP_ATTR_OBJECTSID         _T("objectSid")
#define LDAP_ATTR_USNCREATED        _T("uSNCreated")

#define LDAP_FILTER_SID_FORMAT      _T("(") ## NONE(LDAP_ATTR_OBJECTSID) ## _T("=%s)")
#define LDAP_FILTER_DOMAIN          _T("(") ## NONE(LDAP_ATTR_OBJECT_CLASS) ## _T("=domain)")

#define LDAP_MAX_STR_SID_SIZE       (1 + ((1+10)*5) + 1)// S + (-%u)*5 + null


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _LDAP_OPTIONS {
    PTCHAR  tADLogin;
    PTCHAR  tADPassword;
    PTCHAR  tADExplicitDomain;
    PTCHAR  tLDAPServer;
    DWORD   dwLDAPPort;
    PTCHAR  tDNSName;
} LDAP_OPTIONS, *PLDAP_OPTIONS;

typedef struct _LDAP_CONNECT_INFOS {
    PLDAP   hLDAPConnection;
    PTCHAR  ptLDAPDomainDN;
    PTCHAR  ptLDAPDomainName;
    PTCHAR  ptLDAPDomainSID;
} LDAP_CONNECT_INFOS, *PLDAP_CONNECT_INFO;

typedef DWORD LDAP_REQUESTED_DATA_INFO;

typedef struct _LDAP_RETRIEVED_DATA {
    PTCHAR  tDN;
    PBYTE   *ppbData;
    PDWORD  pdwDataSize;
    DWORD   dwElementCount;
} LDAP_RETRIEVED_DATA, *PLDAP_RETRIEVED_DATA;


/* --- PROTOTYPES ----------------------------------------------------------- */
void LdapParseOptions(
    _Inout_ PLDAP_OPTIONS pLdapOptions,
    _In_ int argc,
    _In_ PTCHAR argv[]
    );

void LdapUsage(
    );

BOOL LdapInit(
    _In_ PTCHAR ptHostName,
    _In_ ULONG dwPortNumber
    );

BOOL LdapBind(
    _In_ PTCHAR ptUserName,
    _In_ PTCHAR ptPassword,
    _In_opt_ PTCHAR ptExplicitDomain
    );

BOOL LdapInitAndBind(
    _In_ PLDAP_OPTIONS pLdapOptions
    );

BOOL LdapDisconnectAndDestroy(
    );

BOOL LdapPagedSearch(
    _In_ PTCHAR tLdapFilter,
    _In_ PTCHAR tOrigAttribute,
    _In_opt_ PLDAPControl *pLdapServerControls,
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    );

BOOL LdapFreePagedSearchResults(
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    );

BOOL LdapExtractAttributes(
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    );

BOOL LdapExtractRangedAttributes(
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tOrigAttribute,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    );

BOOL GetRangeValues(
    _Inout_ PLDAPMessage pEntry,
    _In_ PTCHAR tOriginalAttribute,
    _Inout_ PDWORD pdwAttributeCount,
    _Inout_ PBYTE **pppbData,
    _Inout_ PDWORD *ppdwDataSize
    );

BOOL ParseRange(
    _In_ PTCHAR tAtttype,
    _In_ PTCHAR tAttdescr,
    _Inout_ PDWORD pdwStart,
    _Inout_ PDWORD pdwEnd
    );

BOOL ConstructRangeAtt(
    _In_ PTCHAR tAtttype,
    _In_ DWORD dwStart,
    _In_ INT iEnd,
    _Inout_ PTCHAR* tOutputRangeAttr
    );


BOOL ExtractDomainNamingContext(
    _In_ PLDAP_OPTIONS pLdapOptions
    );

BOOL LdapResolveSid(
    _In_ PTCHAR ptStrSid,
    _Out_ PTCHAR *ptDn
    );

BOOL LdapResolveRid(
    _In_ DWORD dwRid,
    _Out_ PTCHAR *ptDn
    );

BOOL LdapResolveRawSid(
    _In_ PSID pRawSid,
    _Out_ PTCHAR *ptDn
    );

PTCHAR LdapGetDomainNC(
    );

#endif // __LDAP_H__
