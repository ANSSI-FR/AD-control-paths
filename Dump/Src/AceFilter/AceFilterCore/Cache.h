/******************************************************************************\
\******************************************************************************/

#ifndef __CACHE_H__
#define __CACHE_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "AvlTable.h"
#include "ImportedObjects.h"

/* --- DEFINES -------------------------------------------------------------- */
#define AVL_FOREACH(table, ptype, element) \
    ptype element = NULL; \
    for (element = (ptype)RtlEnumerateGenericTableAvl(table, TRUE); \
    element != NULL; \
    element = (ptype)RtlEnumerateGenericTableAvl(table, FALSE) )

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

typedef struct _CACHE_SCHEMA_BY_CLASSID {
    DWORD classid;
    PIMPORTED_SCHEMA schema;
} CACHE_SCHEMA_BY_CLASSID, *PCACHE_SCHEMA_BY_CLASSID;


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
DWORD CacheSchemaByClassidCount(
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
PIMPORTED_SCHEMA CacheLookupSchemaByClassid(
    _In_ DWORD classid
    );

#endif // __CACHE_H__
