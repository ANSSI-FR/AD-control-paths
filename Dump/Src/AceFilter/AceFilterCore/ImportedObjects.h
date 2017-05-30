/******************************************************************************\
\******************************************************************************/

#ifndef __IMPORTED_ACE_H__
#define __IMPORTED_ACE_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "AceConstants.h"


/* --- DEFINES -------------------------------------------------------------- */
#define HELPER_SWITCH_ACE_TYPE_GET(type, val, ace, field, ptr)     case type ## _TYPE: val = ptr((P ## type)(ace))->field; break;

#define HELPER_SWITCH_ACE_TYPE_GET_ALL(val, ace, field, ptr)      \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_CALLBACK_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_CALLBACK_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_CALLBACK_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_CALLBACK_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_ALARM_CALLBACK_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_ALARM_CALLBACK_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_ALARM_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_CALLBACK_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_CALLBACK_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_OBJECT_ACE, val, ace, field, ptr) \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_MANDATORY_LABEL_ACE, val, ace, field, ptr)

#define HELPER_SWITCH_ACE_TYPE_GET_OBJECT(val, ace, field, ptr)      \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_OBJECT_ACE, val, ace, field, ptr); \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_ALLOWED_CALLBACK_OBJECT_ACE, val, ace, field, ptr); \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_OBJECT_ACE, val, ace, field, ptr); \
    HELPER_SWITCH_ACE_TYPE_GET(ACCESS_DENIED_CALLBACK_OBJECT_ACE, val, ace, field, ptr); \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_OBJECT_ACE, val, ace, field, ptr); \
    HELPER_SWITCH_ACE_TYPE_GET(SYSTEM_AUDIT_CALLBACK_OBJECT_ACE, val, ace, field, ptr);

#define IS_OBJECT_ACE(ace)   (\
    (((ace)->AceType) == ACCESS_ALLOWED_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_DENIED_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == SYSTEM_AUDIT_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE) )

#define IS_ALLOWED_ACE(ace)   (\
    (((ace)->AceType) == ACCESS_ALLOWED_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_ALLOWED_CALLBACK_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE) || \
    (((ace)->AceType) == ACCESS_ALLOWED_OBJECT_ACE_TYPE))

/*
cf. http://msdn.microsoft.com/en-us/library/windows/desktop/aa772285(v=vs.85).aspx
- ADS_RIGHT_DS_CREATE_CHILD
- ADS_RIGHT_DS_DELETE_CHILD
- ADS_RIGHT_DS_SELF
- ADS_RIGHT_DS_READ_PROP
- ADS_RIGHT_DS_WRITE_PROP
- ADS_RIGHT_DS_CONTROL_ACCESS
*/
#define OBJECT_TYPE_GUID_NEEDED(mask)   (mask & (ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_SELF|ADS_RIGHT_DS_READ_PROP|ADS_RIGHT_DS_WRITE_PROP|ADS_RIGHT_DS_CONTROL_ACCESS))
#define ACL_ADMIN_SD_HOLDER_SIZE        (10240)

#define IMPORTED_HEAD(obj)              ((PIMPORTED_HEAD)(obj))
#define IMPORTED_REFCOUNT(obj)          IMPORTED_HEAD(obj)->refCount
#define IMPORTED_INCREF(obj)            IMPORTED_REFCOUNT(obj) += 1;
#define IMPORTED_DECREF(obj)            MULTI_LINE_MACRO_BEGIN                  \
                                            if (IMPORTED_REFCOUNT(obj) > 0) {   \
                                                IMPORTED_REFCOUNT(obj) -= 1;    \
                                            }                                   \
                                        MULTI_LINE_MACRO_END
#define IMPORTED_FIELD_CST_CPY(d,s,f)   (d)->f = (s)->f;
#define IMPORTED_FIELD_BUF_CPY(d,s,f,l) memcpy(&((d)->f), &((s)->f), l);
#define IMPORTED_FIELD_DUP_CPY(d,s,f)   (d)->f = UtilsHeapStrDupHelper(gs_hHeapCache,(s)->f);
#define IMPORTED_FIELD_ARR_CPY(d,s,f,c) MULTI_LINE_MACRO_BEGIN                                        \
                                          (d)->f = UtilsHeapAllocArrayHelper(gs_hHeapCache,LPTSTR,c); \
                                          for (unsigned int i = 0;i<c;i++)                            \
                                            IMPORTED_FIELD_DUP_CPY(d,s,f[i]);                         \
                                        MULTI_LINE_MACRO_END

/* --- TYPES ---------------------------------------------------------------- */
typedef enum _ACE_SOURCE {
    AceFromActiveDirectory,
    AceFromFileSystem,
    // TODO : add other sources
} ACE_SOURCE;

typedef struct _WELLKNOWN_BIGRAMS {
	LPTSTR bigram;
	LPTSTR rid;
} WELLKNOWN_BIGRAMS, *WELLKNOWN_PBIGRAMS;

typedef struct _IMPORTED_HEAD {
    DWORD refCount;
} IMPORTED_HEAD, *PIMPORTED_HEAD;

typedef struct _IMPORTED_SCHEMA {

    IMPORTED_HEAD head;

    struct {
        LPTSTR dn;
        GUID schemaIDGUID;
        DWORD governsID;
        LPTSTR defaultSecurityDescriptor;
		LPTSTR lDAPDisplayName;
    } imported;

    struct {
        struct _IMPORTED_OBJECT * object;
    } resolved;

    struct {
        DWORD number;
		PSECURITY_DESCRIPTOR defaultSD;
    } computed;

#ifdef _DEBUG
    struct {
        DWORD whatever;
    } debug;
#endif

} IMPORTED_SCHEMA, *PIMPORTED_SCHEMA;


typedef struct _IMPORTED_OBJECT {

    IMPORTED_HEAD head;

    struct {
        LPTSTR dn;
        BYTE sid[SECURITY_MAX_SID_SIZE];
        BYTE adminCount;
		LPTSTR *objectClassesNames;
		LPTSTR mail;
    } imported;

    struct {
        PIMPORTED_SCHEMA * objectClasses;
    } resolved;

    struct {
        DWORD objectClassCount;
        DWORD sidLength;
        DWORD number;
    } computed;

#ifdef _DEBUG
    struct {
        DWORD whatever;
    } debug;
#endif

} IMPORTED_OBJECT, *PIMPORTED_OBJECT;


typedef struct _IMPORTED_ACE {

    struct {
        LPTSTR objectDn;
        PACE_HEADER raw;
        ACE_SOURCE source;
    } imported;

    struct {
        PIMPORTED_OBJECT object;
        PIMPORTED_OBJECT trustee;
		LPTSTR mail;
    } resolved;

    struct {
        DECLARE_BITMAP(relation, ACE_REL_COUNT);
        DWORD number;
        LPTSTR trusteeStr;
    } computed;

#ifdef _DEBUG
    struct {
        DWORD whatever;
    } debug;
#endif

} IMPORTED_ACE, *PIMPORTED_ACE;


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
ACCESS_MASK GetAccessMask(
    _In_ PIMPORTED_ACE impace
    );

PSID GetTrustee(
    _In_ PIMPORTED_ACE impace
    );

DWORD GetObjectFlags(
    _In_ PIMPORTED_ACE impace
    );

GUID *GetObjectTypeAce(
    _In_ PIMPORTED_ACE impace
    );

GUID *GetInheritedObjectTypeAce(
    _In_ PIMPORTED_ACE impace
    );

void InitAdminSdHolderSd(
    );

void AddAceToAdminSdHolderSd(
    _In_ PIMPORTED_ACE ace
    );

BOOL IsInAdminSdHolder(
    _In_ PIMPORTED_ACE ace
    );

BOOL IsInDefaultSd(
    _In_ PIMPORTED_ACE ace
    );

BOOL isObjectTypeClass(
	_In_ PIMPORTED_ACE ace
	);

BOOL isObjectTypeClassMatching(
	_In_ PIMPORTED_ACE ace
	);

LPTSTR ResolverGetAceTrusteeStr(
    _In_ PIMPORTED_ACE ace
    );

PIMPORTED_OBJECT  ResolverGetAceTrustee(
    _In_ PIMPORTED_ACE ace
    );

PIMPORTED_OBJECT ResolverGetAceObject(
    _In_ PIMPORTED_ACE ace
    );

LPTSTR ResolverGetAceObjectMail(
	_In_ PIMPORTED_ACE ace
);

PSECURITY_DESCRIPTOR ResolverGetSchemaDefaultSD(
    _In_ PIMPORTED_SCHEMA sch
    );

PIMPORTED_OBJECT ResolverGetSchemaObject(
    _In_ PIMPORTED_SCHEMA sch
    );

PIMPORTED_OBJECT GetObjectByDn(
    _In_ LPTSTR dn
    );

PIMPORTED_OBJECT GetObjectBySid(
    _In_ PSID sid
    );

PIMPORTED_SCHEMA GetSchemaByGuid(
    _In_ GUID * guid
    );

PIMPORTED_SCHEMA GetSchemaByDisplayName(
	_In_ LPTSTR displayname
	);

LPTSTR GetDomainDn(
    );

GUID *GetAdmPwdGuid(
);

void CacheActivateObjectCache(
    );

void CacheActivateSchemaCache(
    );

// Cache.h cannot be included here (circular inclusion), so we just declare the used functions here :

extern PIMPORTED_OBJECT CacheLookupObjectBySid(
    _In_ PSID sid
    );

extern PIMPORTED_OBJECT CacheLookupObjectByDn(
    _In_ LPTSTR dn
    );

extern PIMPORTED_SCHEMA CacheLookupSchemaByGuid(
    _In_ GUID * guid
    );

extern PIMPORTED_SCHEMA CacheLookupSchemaByDisplayName(
	_In_ LPTSTR displayname
	);

extern LPTSTR CacheGetDomainDn(
    );

extern GUID *CacheGetAdmPwdGuid(
    );

BOOL IsAceInSd(
	_In_ PACE_HEADER pHeaderAceToCheck,
	_In_ PSECURITY_DESCRIPTOR pSd
);

#endif // __IMPORTED_ACE_H__
