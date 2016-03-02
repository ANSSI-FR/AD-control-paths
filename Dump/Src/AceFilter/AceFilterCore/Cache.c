/******************************************************************************\
\******************************************************************************/


/* --- INCLUDES ------------------------------------------------------------- */
#include "Cache.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */

static HANDLE gs_hHeapCache = INVALID_HANDLE_VALUE;
static RTLINITIALIZEGENERICTABLEAVL RtlInitializeGenericTableAvl = NULL;
static RTLINSERTELEMENTGENERICTABLEAVL RtlInsertElementGenericTableAvl = NULL;
static RTLLOOKUPELEMENTGENERICTABLEAVL RtlLookupElementGenericTableAvl = NULL;
static RTLENUMERATEGENERICTABLEAVL RtlEnumerateGenericTableAvl = NULL;
static RTLDELETEGENERICTABLEAVL RtlDeleteElementGenericTableAvl = NULL;

// Object cache 
static DWORD gs_ObjectBySidCount = 0;
static DWORD gs_ObjectByDnCount = 0;
static RTL_AVL_TABLE gs_AvlTableObjectBySid = { 0 };
static RTL_AVL_TABLE gs_AvlTableObjectByDn = { 0 };

// Schema cache
static DWORD gs_SchemaByGuidCount = 0;
static DWORD gs_SchemaByClassidCount = 0;
static DWORD gs_SchemaByDisplayNameCount = 0;
static RTL_AVL_TABLE gs_AvlTableSchemaByGuid = { 0 };
static RTL_AVL_TABLE gs_AvlTableSchemaByClassid = { 0 };
static RTL_AVL_TABLE gs_AvlTableSchemaByDisplayName = { 0 };


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static PVOID _Function_class_(RTL_AVL_ALLOCATE_ROUTINE) NTAPI AvlAllocate(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  CLONG                   ByteSize
    ) {
    UNREFERENCED_PARAMETER(Table);
    return HeapAllocCheckX(gs_hHeapCache, HEAP_ZERO_MEMORY, ByteSize);
}

static VOID _Function_class_(RTL_AVL_FREE_ROUTINE) NTAPI AvlFree(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   Buffer
    ) {
    UNREFERENCED_PARAMETER(Table);
    HeapFreeCheckX(gs_hHeapCache, 0, Buffer);
}

static RTL_GENERIC_COMPARE_RESULTS CompareNums(
    _In_ int First,
    _In_ int Second
    ) {
    if (First < Second)
        return GenericLessThan;

    if (First > Second)
        return GenericGreaterThan;

    return GenericEqual;
}

static RTL_GENERIC_COMPARE_RESULTS CompareBinStruct(
    _In_ PVOID First,
    _In_ PVOID Second,
    _In_ DWORD Len
    ) {
    return CompareNums(memcmp(First, Second, Len), 0);
}

static RTL_GENERIC_COMPARE_RESULTS CompareStr(
    LPTSTR First,
    LPTSTR Second
    ) {
    return CompareNums(_tcsicmp(First, Second), 0);
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareObjectSid(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);

    PSID psid1 = &((PCACHE_OBJECT_BY_SID)(FirstStruct))->sid;
    PSID psid2 = &((PCACHE_OBJECT_BY_SID)(SecondStruct))->sid;
    DWORD len1 = GetLengthSid(psid1);
    DWORD len2 = GetLengthSid(psid2);
    RTL_GENERIC_COMPARE_RESULTS cmp = CompareNums(len1, len2);

    if (cmp == GenericEqual) {
        cmp = CompareBinStruct(psid1, psid2, len1);
    }
    return cmp;
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareObjectDn(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);
    return CompareStr(((PCACHE_OBJECT_BY_DN)(FirstStruct))->dn, ((PCACHE_OBJECT_BY_DN)(SecondStruct))->dn);
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareSchemaGuid(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);
    return CompareBinStruct(&((PCACHE_SCHEMA_BY_GUID)(FirstStruct))->guid, &((PCACHE_SCHEMA_BY_GUID)(SecondStruct))->guid, sizeof(GUID));
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareSchemaClassid(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);
    return CompareNums(((PCACHE_SCHEMA_BY_CLASSID)(FirstStruct))->classid, ((PCACHE_SCHEMA_BY_CLASSID)(SecondStruct))->classid);
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareSchemaDisplayName(
	_In_  struct _RTL_AVL_TABLE   *Table,
	_In_  PVOID                   FirstStruct,
	_In_  PVOID                   SecondStruct
	) {
	UNREFERENCED_PARAMETER(Table);
	return CompareStr(((PCACHE_SCHEMA_BY_DISPLAYNAME)(FirstStruct))->displayname, ((PCACHE_SCHEMA_BY_DISPLAYNAME)(SecondStruct))->displayname);
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
BOOL CachesInitialize(
    ) {
    HMODULE hNtdll = 0;

    //
    // AVL cache heap
    //
    gs_hHeapCache = HeapCreate(0, 0, 0);
    if (!gs_hHeapCache) {
        LOG(Err, _T("cannot create cache's heap : <%u>"), GetLastError());
        return FALSE;
    }

    //
    // AVL ntdll functions
    //
    hNtdll = GetModuleHandle(_T("ntdll.dll"));
    if (!hNtdll) {
        FATAL(_T("Cannot get a handle on ntdll"));
    }

    RtlInitializeGenericTableAvl = (RTLINITIALIZEGENERICTABLEAVL)GetProcAddress(hNtdll, "RtlInitializeGenericTableAvl");
    RtlInsertElementGenericTableAvl = (RTLINSERTELEMENTGENERICTABLEAVL)GetProcAddress(hNtdll, "RtlInsertElementGenericTableAvl");
    RtlLookupElementGenericTableAvl = (RTLLOOKUPELEMENTGENERICTABLEAVL)GetProcAddress(hNtdll, "RtlLookupElementGenericTableAvl");
    RtlEnumerateGenericTableAvl = (RTLENUMERATEGENERICTABLEAVL)GetProcAddress(hNtdll, "RtlEnumerateGenericTableAvl");
    RtlDeleteElementGenericTableAvl = (RTLDELETEGENERICTABLEAVL)GetProcAddress(hNtdll, "RtlDeleteElementGenericTableAvl");

    if (!RtlInitializeGenericTableAvl
        || !RtlInsertElementGenericTableAvl
        || !RtlLookupElementGenericTableAvl
        || !RtlEnumerateGenericTableAvl
        || !RtlDeleteElementGenericTableAvl) {
        FATAL(_T("Cannot resolve AVL functions"));
    }

    //
    // AVL Tables
    //
    RtlInitializeGenericTableAvl(&gs_AvlTableObjectBySid, AvlCompareObjectSid, AvlAllocate, AvlFree, NULL);
    RtlInitializeGenericTableAvl(&gs_AvlTableObjectByDn, AvlCompareObjectDn, AvlAllocate, AvlFree, NULL);
    RtlInitializeGenericTableAvl(&gs_AvlTableSchemaByGuid, AvlCompareSchemaGuid, AvlAllocate, AvlFree, NULL);
    RtlInitializeGenericTableAvl(&gs_AvlTableSchemaByClassid, AvlCompareSchemaClassid, AvlAllocate, AvlFree, NULL);
	RtlInitializeGenericTableAvl(&gs_AvlTableSchemaByDisplayName, AvlCompareSchemaDisplayName, AvlAllocate, AvlFree, NULL);

    return TRUE;
}

void CachesDestroy(
    ) {
    // TODO : Poorly written. Some code can be refactored.
    PIMPORTED_OBJECT impObj = NULL;
    PIMPORTED_SCHEMA impSch = NULL;

    AVL_FOREACH(&gs_AvlTableObjectBySid, PCACHE_OBJECT_BY_SID, cacheSid) {
        impObj = cacheSid->object;
        RtlDeleteElementGenericTableAvl(&gs_AvlTableObjectBySid, cacheSid);
        IMPORTED_DECREF(impObj);
        if (IMPORTED_REFCOUNT(impObj) == 0) {
            FreeCheckX(impObj->imported.dn);
            HeapFreeCheckX(gs_hHeapCache, 0, impObj);
        }
    }

    AVL_FOREACH(&gs_AvlTableObjectByDn, PCACHE_OBJECT_BY_DN, cacheDn) {
        impObj = cacheDn->object;
        RtlDeleteElementGenericTableAvl(&gs_AvlTableObjectByDn, cacheDn);
        IMPORTED_DECREF(impObj);
        if (IMPORTED_REFCOUNT(impObj) == 0) {
            FreeCheckX(impObj->imported.dn);
            HeapFreeCheckX(gs_hHeapCache, 0, impObj);
        }
    }

    AVL_FOREACH(&gs_AvlTableSchemaByGuid, PCACHE_SCHEMA_BY_GUID, cacheGuid) {
        impSch = cacheGuid->schema;
        RtlDeleteElementGenericTableAvl(&gs_AvlTableSchemaByGuid, cacheGuid);
        IMPORTED_DECREF(impSch);
        if (IMPORTED_REFCOUNT(impSch) == 0) {
            FreeCheckX(impSch->imported.dn);
            HeapFreeCheckX(gs_hHeapCache, 0, impSch);
        }
    }

    AVL_FOREACH(&gs_AvlTableSchemaByClassid, PCACHE_SCHEMA_BY_CLASSID, cacheClassid) {
        impSch = cacheClassid->schema;
        RtlDeleteElementGenericTableAvl(&gs_AvlTableSchemaByClassid, cacheClassid);
        IMPORTED_DECREF(impSch);
        if (IMPORTED_REFCOUNT(impSch) == 0) {
            FreeCheckX(impSch->imported.dn);
            HeapFreeCheckX(gs_hHeapCache, 0, impSch);
        }
    }

	AVL_FOREACH(&gs_AvlTableSchemaByDisplayName, PCACHE_SCHEMA_BY_DISPLAYNAME, cacheDisplayName) {
		impSch = cacheDisplayName->schema;
		RtlDeleteElementGenericTableAvl(&gs_AvlTableSchemaByDisplayName, cacheDisplayName);
		IMPORTED_DECREF(impSch);
		if (IMPORTED_REFCOUNT(impSch) == 0) {
			FreeCheckX(impSch->imported.dn);
			HeapFreeCheckX(gs_hHeapCache, 0, impSch);
		}
	}

    HeapDestroy(gs_hHeapCache);
}

DWORD CacheObjectBySidCount(
    ) {
    return gs_ObjectBySidCount;
}

DWORD CacheObjectByDnCount(
    ) {
    return gs_ObjectByDnCount;
}

DWORD CacheSchemaByGuidCount(
    ) {
    return gs_SchemaByGuidCount;
}

DWORD CacheSchemaByClassidCount(
    ) {
    return gs_SchemaByClassidCount;
}

DWORD CacheSchemaByDisplayNameCount(
	) {
	return gs_SchemaByDisplayNameCount;
}

void CacheInsertObject(
    _In_ PIMPORTED_OBJECT obj
    ) {
    PIMPORTED_OBJECT object = (PIMPORTED_OBJECT)HeapAllocCheckX(gs_hHeapCache, HEAP_ZERO_MEMORY, sizeof(IMPORTED_OBJECT));
    IMPORTED_FIELD_CST_CPY(object, obj, computed.sidLength);
    IMPORTED_FIELD_CST_CPY(object, obj, computed.number);
	IMPORTED_FIELD_CST_CPY(object, obj, computed.objectClassCount);
    IMPORTED_FIELD_BUF_CPY(object, obj, imported.sid, obj->computed.sidLength);
    IMPORTED_FIELD_DUP_CPY(object, obj, imported.dn);
	IMPORTED_FIELD_CST_CPY(object, obj, imported.objectClassesNames);

    //
    // Cache Object-By-Sid
    //
    if (object->computed.sidLength && !SID_EMPTY(&object->imported.sid, object->computed.sidLength)) {
        CACHE_OBJECT_BY_SID cacheEntry = { 0 };
        PCACHE_OBJECT_BY_SID inserted = NULL;
        BOOLEAN newElement = FALSE;

        RtlCopyMemory(cacheEntry.sid, &object->imported.sid, object->computed.sidLength);
        cacheEntry.object = object;

        inserted = (PCACHE_OBJECT_BY_SID)RtlInsertElementGenericTableAvl(&gs_AvlTableObjectBySid, (PVOID)&cacheEntry, sizeof(CACHE_OBJECT_BY_SID), &newElement);
        if (!inserted) {
            LOG(Err, _T("cannot insert new object-by-sid cache entry <%u : %s>"), object->computed.number, object->imported.dn);
        }
        else if (!newElement) {
            LOG(Dbg, _T("object-by-sid cache entry is not new <%u : %s>"), object->computed.number, object->imported.dn);
        }
        else {
            IMPORTED_INCREF(object);
            gs_ObjectBySidCount++;
        }
    }
    else {
        LOG(All, _T("Object entry <%u> does not have a SID"), object->computed.number);
    }

    //
    // Cache Object-By-Dn
    //
    if (!STR_EMPTY(object->imported.dn)) {
        CACHE_OBJECT_BY_DN cacheEntry = { 0 };
        PCACHE_OBJECT_BY_DN inserted = NULL;
        BOOLEAN newElement = FALSE;

        cacheEntry.dn = object->imported.dn;
        cacheEntry.object = object;

        inserted = (PCACHE_OBJECT_BY_DN)RtlInsertElementGenericTableAvl(&gs_AvlTableObjectByDn, (PVOID)&cacheEntry, sizeof(CACHE_OBJECT_BY_DN), &newElement);
        if (!inserted) {
            LOG(Err, _T("cannot insert new object-by-dn cache entry <%u : %s>"), object->computed.number, object->imported.dn);
        }
        else if (!newElement) {
            LOG(Dbg, _T("object-by-dn cache entry is not new <%u : %s>"), object->computed.number, object->imported.dn);
        }
        else {
            IMPORTED_INCREF(object);
            gs_ObjectByDnCount++;
        }
    }
    else {
        LOG(All, _T("Object entry <%u> does not have a DN"), object->computed.number);
    }

    if (IMPORTED_REFCOUNT(object) == 0) {
        HeapFreeCheckX(gs_hHeapCache, 0, object);
    }
}

void CacheInsertSchema(
    _In_ PIMPORTED_SCHEMA sch
    ) {
    PIMPORTED_SCHEMA schema = (PIMPORTED_SCHEMA)HeapAllocCheckX(gs_hHeapCache, HEAP_ZERO_MEMORY, sizeof(IMPORTED_SCHEMA));
    IMPORTED_FIELD_CST_CPY(schema, sch, imported.governsID);
    IMPORTED_FIELD_CST_CPY(schema, sch, computed.number);
    IMPORTED_FIELD_BUF_CPY(schema, sch, imported.schemaIDGUID, sizeof(GUID));
    IMPORTED_FIELD_DUP_CPY(schema, sch, imported.dn);
	IMPORTED_FIELD_DUP_CPY(schema, sch, imported.lDAPDisplayName);

    //
    // Cache Schema-By-Guid
    //
    if (!GUID_EMPTY(&schema->imported.schemaIDGUID)) {
        CACHE_SCHEMA_BY_GUID cacheEntry = { 0 };
        PCACHE_SCHEMA_BY_GUID inserted = NULL;
        BOOLEAN newElement = FALSE;

        cacheEntry.guid = schema->imported.schemaIDGUID;
        cacheEntry.schema = schema;

        inserted = (PCACHE_SCHEMA_BY_GUID)RtlInsertElementGenericTableAvl(&gs_AvlTableSchemaByGuid, (PVOID)&cacheEntry, sizeof(CACHE_SCHEMA_BY_GUID), &newElement);
        if (!inserted) {
            LOG(Err, _T("cannot insert new schema-by-guid cache entry <%u : %s>"), schema->computed.number, schema->imported.dn);
        }
        else if (!newElement) {
            LOG(Dbg, _T("schema-by-guid cache entry is not new <%u : %s>"), schema->computed.number, schema->imported.dn);
        }
        else {
            IMPORTED_INCREF(schema);
            gs_SchemaByGuidCount++;
        }
    }
    else {
        LOG(All, _T("Schema entry <%u> does not have a GUID"), schema->computed.number);
    }

    //
    // Cache Schema-By-Classid
    //
    if (schema->imported.governsID != 0) {
        CACHE_SCHEMA_BY_CLASSID cacheEntry = { 0 };
        PCACHE_SCHEMA_BY_CLASSID inserted = NULL;
        BOOLEAN newElement = FALSE;

        cacheEntry.classid = schema->imported.governsID;
        cacheEntry.schema = schema;

        inserted = (PCACHE_SCHEMA_BY_CLASSID)RtlInsertElementGenericTableAvl(&gs_AvlTableSchemaByClassid, (PVOID)&cacheEntry, sizeof(CACHE_SCHEMA_BY_GUID), &newElement);
        if (!inserted) {
            LOG(Err, _T("cannot insert new schema-by-classid cache entry <%u : %s>"), schema->computed.number, schema->imported.dn);
        }
        else if (!newElement) {
            LOG(Dbg, _T("schema-by-classid cache entry is not new <%u : %s>"), schema->computed.number, schema->imported.dn);
        }
        else {
            IMPORTED_INCREF(schema);
            gs_SchemaByClassidCount++;
        }
    }
    else {
        LOG(All, _T("Schema entry <%u> does not have a governsID"), schema->computed.number);
    }

	//
	// Cache Schema-By-DisplayName
	//
	if (!GUID_EMPTY(&schema->imported.lDAPDisplayName)) {
		CACHE_SCHEMA_BY_DISPLAYNAME cacheEntry = { 0 };
		PCACHE_SCHEMA_BY_DISPLAYNAME inserted = NULL;
		BOOLEAN newElement = FALSE;

		cacheEntry.displayname = schema->imported.lDAPDisplayName;
		cacheEntry.schema = schema;

		inserted = (PCACHE_SCHEMA_BY_DISPLAYNAME)RtlInsertElementGenericTableAvl(&gs_AvlTableSchemaByDisplayName, (PVOID)&cacheEntry, sizeof(CACHE_SCHEMA_BY_DISPLAYNAME), &newElement);
		if (!inserted) {
			LOG(Err, _T("cannot insert new schema-by-displayname cache entry <%u : %s>"), schema->computed.number, schema->imported.dn);
		}
		else if (!newElement) {
			LOG(Dbg, _T("schema-by-displayname cache entry is not new <%u : %s>"), schema->computed.number, schema->imported.dn);
		}
		else {
			IMPORTED_INCREF(schema);
			gs_SchemaByDisplayNameCount++;
		}
	}
	else {
		LOG(All, _T("Schema entry <%u> does not have a DisplayName"), schema->computed.number);
	}


    if (IMPORTED_REFCOUNT(schema) == 0) {
        HeapFreeCheckX(gs_hHeapCache, 0, schema);
    }
}

PIMPORTED_OBJECT CacheLookupObjectBySid(
    _In_ PSID sid
    ) {
    CACHE_OBJECT_BY_SID searched = { 0 };
    PCACHE_OBJECT_BY_SID returned = NULL;

    RtlCopyMemory(&searched.sid, sid, GetLengthSid(sid));
    returned = (PCACHE_OBJECT_BY_SID)RtlLookupElementGenericTableAvl(&gs_AvlTableObjectBySid, (PVOID)&searched);

    if (!returned) {
        LOG(All, _T("cannot find object-by-sid entry for <%s>"), _T("TODO"));
        return NULL;
    }

    return returned->object;
}

PIMPORTED_OBJECT CacheLookupObjectByDn(
    _In_ LPTSTR dn
    ) {
    CACHE_OBJECT_BY_DN searched = { 0 };
    PCACHE_OBJECT_BY_DN returned = NULL;

    searched.dn = dn;
    returned = (PCACHE_OBJECT_BY_DN)RtlLookupElementGenericTableAvl(&gs_AvlTableObjectByDn, (PVOID)&searched);

    if (!returned) {
        LOG(Dbg, _T("cannot find object-by-dn entry for <%s>"), dn);
        return NULL;
    }

    return returned->object;
}

PIMPORTED_SCHEMA CacheLookupSchemaByGuid(
    _In_ GUID * guid
    ) {
    CACHE_SCHEMA_BY_GUID searched = { 0 };
    PCACHE_SCHEMA_BY_GUID returned = NULL;

    RtlCopyMemory(&searched.guid, guid, sizeof(GUID));
    returned = (PCACHE_SCHEMA_BY_GUID)RtlLookupElementGenericTableAvl(&gs_AvlTableSchemaByGuid, (PVOID)&searched);

    if (!returned) {
        LOG(All, _T("cannot find schema-by-guid entry for <%s>"), "TODO");
        return NULL;
    }

    return returned->schema;
}

PIMPORTED_SCHEMA CacheLookupSchemaByClassid(
    _In_ DWORD classid
    ) {
    CACHE_SCHEMA_BY_CLASSID searched = { 0 };
    PCACHE_SCHEMA_BY_CLASSID returned = NULL;

    searched.classid = classid;
    returned = (PCACHE_SCHEMA_BY_CLASSID)RtlLookupElementGenericTableAvl(&gs_AvlTableSchemaByClassid, (PVOID)&searched);

    if (!returned) {
        LOG(Dbg, _T("cannot find schema-by-classid entry for <%d>"), classid);
        return NULL;
    }

    return returned->schema;
}


PIMPORTED_SCHEMA CacheLookupSchemaByDisplayName(
	_In_ LPTSTR displayname
	) {
	CACHE_SCHEMA_BY_DISPLAYNAME searched = { 0 };
	PCACHE_SCHEMA_BY_DISPLAYNAME returned = NULL;

	searched.displayname = displayname;
	returned = (PCACHE_SCHEMA_BY_DISPLAYNAME)RtlLookupElementGenericTableAvl(&gs_AvlTableSchemaByDisplayName, (PVOID)&searched);

	if (!returned) {
		LOG(Dbg, _T("cannot find schema-by-displayname entry for <%s>"), displayname);
		return NULL;
	}

	return returned->schema;
}

LPTSTR CacheGetDomainDn(
    ) {
    LPTSTR domain = NULL;

    AVL_FOREACH(&gs_AvlTableObjectByDn, PCACHE_OBJECT_BY_DN, cacheDn) {
        domain = _tcsstr(cacheDn->dn, _T("DC="));
        if (domain) {
            return StrDupCheckX(domain);
        }
    }

    return NULL;
}