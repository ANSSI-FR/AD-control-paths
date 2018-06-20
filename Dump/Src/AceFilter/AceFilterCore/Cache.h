/******************************************************************************\
\******************************************************************************/

#ifndef __CACHE_H__
#define __CACHE_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "ImportedObjects.h"
#include "CacheLib.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CACHE_HEAP_NAME _T("CACHEHEAP")


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _CACHE_OBJECT_BY_SID {
	BYTE sid[SECURITY_MAX_SID_SIZE];
	PIMPORTED_OBJECT object;
} CACHE_OBJECT_BY_SID, *PCACHE_OBJECT_BY_SID;

typedef struct _CACHE_OBJECT_BY_DN {
	LPTSTR dn;
	PIMPORTED_OBJECT object;
} CACHE_OBJECT_BY_DN, *PCACHE_OBJECT_BY_DN;

typedef struct _CACHE_SCHEMA_BY_GUID {
	GUID guid;
	PIMPORTED_SCHEMA schema;
} CACHE_SCHEMA_BY_GUID, *PCACHE_SCHEMA_BY_GUID;

typedef struct _CACHE_SCHEMA_BY_DISPLAYNAME {
	LPTSTR displayname;
	PIMPORTED_SCHEMA schema;
} CACHE_SCHEMA_BY_DISPLAYNAME, *PCACHE_SCHEMA_BY_DISPLAYNAME;


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
BOOL CachesInitialize(
    );
void CachesDestroy(
    );
DWORD CacheObjectBySidCount(
    );
DWORD CacheObjectByDnCount(
    );
DWORD CacheSchemaByGuidCount(
    );
DWORD CacheSchemaByDisplayNameCount(
	);
void CacheInsertObject(
    _In_ PIMPORTED_OBJECT obj
    );
void CacheInsertSchema(
    _In_ PIMPORTED_SCHEMA sch
    );


PIMPORTED_OBJECT CacheLookupObjectBySid(
    _In_ PSID sid
    );
PIMPORTED_OBJECT CacheLookupObjectByDn(
    _In_ LPTSTR dn
    );
PIMPORTED_SCHEMA CacheLookupSchemaByGuid(
    _In_ GUID * guid
    );
PIMPORTED_SCHEMA CacheLookupSchemaByDisplayName(
	_In_ LPTSTR displayname
	);

LPTSTR CacheGetDomainDn(
    );
GUID *CacheGetAdmPwdGuid(
    );

static void _Function_class_(const PFN_CACHE_ENTRY_DESTROY_CALLBACK) pfnObjEntryDestroy(
	_In_ const PCACHE ppCache,
	_In_ const PVOID pvEntry
);
static void _Function_class_(const PFN_CACHE_ENTRY_DESTROY_CALLBACK) pfnSchEntryDestroy(
	_In_ const PCACHE ppCache,
	_In_ const PVOID pvEntry
);
#endif // __CACHE_H__
