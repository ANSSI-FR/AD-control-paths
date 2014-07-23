#ifndef __LDAP_H__
#define __LDAP_H__


/* --- INCLUDES ------------------------------------------------------------- */
#include "Utils.h"
#include <Winldap.h>
#include <WinBer.h>
#include <NtLdap.h>


/* --- DEFINES -------------------------------------------------------------- */
#define AD_LDAP_SEARCH_LIMIT        (1)
#define DEFAULT_LDAP_PORT           (389)

#define LDAP_ATTR_DISTINGUISHEDNAME _T("dn")
#define LDAP_ATTR_COMMONNAME        _T("cn")
#define LDAP_ATTR_GPLINK            _T("gPLink")
#define LDAP_ATTR_OBJECT_CLASS      _T("objectClass")
#define LDAP_ATTR_MEMBER            _T("member")
#define LDAP_ATTR_NTSD              _T("nTSecurityDescriptor")
#define LDAP_ATTR_OBJECT_SID        _T("objectSid")
#define LDAP_ATTR_ADMIN_COUNT       _T("adminCount")
#define LDAP_ATTR_SCHEMA_IDGUID     _T("schemaIDGUID")
#define LDAP_ATTR_GOVERNS_ID        _T("governsID")
#define LDAP_ATTR_DEFAULT_SD        _T("defaultSecurityDescriptor")

#define LDAP_FILTER_SID_FORMAT      _T("(") ## NONE(LDAP_ATTR_OBJECTSID) ## _T("=%s)")


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
    PTCHAR  ptLDAPConfigDN;
    PTCHAR  ptLDAPSchemaDN;
    PTCHAR  ptLDAPDomainName;
    PTCHAR  ptLDAPRestrictedBindingDN;
} LDAP_CONNECT_INFOS, *PLDAP_CONNECT_INFO;

typedef DWORD LDAP_REQUESTED_DATA_INFO;

typedef struct _LDAP_RETRIEVED_DATA {
    PTCHAR  tDN;
    PBYTE   *ppbData;
    PDWORD  pdwDataSize;
    DWORD   dwElementCount;
} LDAP_RETRIEVED_DATA, *PLDAP_RETRIEVED_DATA;


typedef struct _LDAP_RESULT_ATTRIBUTE {
    DWORD dwValuesCount;
    PDWORD pdwValuesSize;
    PBYTE *ppbValuesData;
} LDAP_RESULT_ATTRIBUTE, *PLDAP_RESULT_ATTRIBUTE;

typedef struct _LDAP_RESULT_ENTRY {
    PTCHAR ptDn;
    DWORD dwAttrsCount;
    PLDAP_RESULT_ATTRIBUTE *pLdapAttrs;
} LDAP_RESULT_ENTRY, *PLDAP_RESULT_ENTRY;

typedef struct _LDAP_FULL_RESULT {
    DWORD dwResultCount;
    PLDAP_RESULT_ENTRY *pResults;
} LDAP_FULL_RESULT, *PLDAP_FULL_RESULT;


/* --- VARIABLES ------------------------------------------------------------ */
extern HANDLE hLdapHeap;


/* --- PROTOTYPES ----------------------------------------------------------- */
BOOL LdapInit(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    );

BOOL LdapBind(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PTCHAR ptNamingContext,
    _In_ PTCHAR ptUserName,
    _In_ PTCHAR ptPassword,
    _In_opt_ PTCHAR ptExplicitDomain
    );

BOOL LdapDisconnectAndDestroy(
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    );

BOOL LdapPagedSearch(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PTCHAR tLdapBaseDn,
    _In_ PTCHAR tLdapFilter,
    _In_ PTCHAR tOrigAttribute,
    _In_ PLDAPControl *pLdapServerControls,
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    );

BOOL LdapFreePagedSearchResults(
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    );

BOOL LdapExtractAttributes(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    );

BOOL LdapExtractRangedAttributes(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tOrigAttribute,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    );

BOOL GetRangeValues(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
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
    _Out_ PTCHAR* tOutputRangeAttr
    );

BOOL ExtractDomainNamingContexts(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    );

BOOL LdapPagedSearchFullResult(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR ptLdapConnectNC,
    _In_ LPTSTR ptLdapDumpDN,
    _In_ LPTSTR ptLdapFilter,
    _In_ PLDAPControl *pLdapServerControls,
    _In_ const LPTSTR ptLdapAttributesNames[],
    _In_ DWORD dwLdapAttributesCount,
    _In_ PLDAP_FULL_RESULT pLdapFullResults
    );

void LdapDestroyResults(
    _Inout_ PLDAP_FULL_RESULT pLdapResults
    );


#endif // __LDAP_H__
