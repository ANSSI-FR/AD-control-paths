/******************************************************************************\
\******************************************************************************/


/* --- INCLUDES ------------------------------------------------------------- */
#include "Cache.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PUTILS_HEAP gs_hHeapCache = INVALID_HANDLE_VALUE;


// Object cache 
static DWORD gs_ObjectBySidCount = 0;
static DWORD gs_ObjectByDnCount = 0;
static PCACHE gs_CacheObjectBySid = NULL;
static PCACHE gs_CacheObjectByDn = NULL;

// Schema cache
static DWORD gs_SchemaByGuidCount = 0;
static DWORD gs_SchemaByDisplayNameCount = 0;
static PCACHE gs_CacheSchemaByGuid = NULL;
static PCACHE gs_CacheSchemaByDisplayName = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
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
    RTL_GENERIC_COMPARE_RESULTS cmp = CacheCompareNums(len1, len2);

    if (cmp == GenericEqual) {
        cmp = CacheCompareBinStruct(psid1, psid2, len1);
    }
    return cmp;
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareObjectDn(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);
    return CacheCompareStr(((PCACHE_OBJECT_BY_DN)(FirstStruct))->dn, ((PCACHE_OBJECT_BY_DN)(SecondStruct))->dn);
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareSchemaGuid(
    _In_  struct _RTL_AVL_TABLE   *Table,
    _In_  PVOID                   FirstStruct,
    _In_  PVOID                   SecondStruct
    ) {
    UNREFERENCED_PARAMETER(Table);
    return CacheCompareBinStruct(&((PCACHE_SCHEMA_BY_GUID)(FirstStruct))->guid, &((PCACHE_SCHEMA_BY_GUID)(SecondStruct))->guid, sizeof(GUID));
}

static RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE) NTAPI AvlCompareSchemaDisplayName(
	_In_  struct _RTL_AVL_TABLE   *Table,
	_In_  PVOID                   FirstStruct,
	_In_  PVOID                   SecondStruct
	) {
	UNREFERENCED_PARAMETER(Table);
	return CacheCompareStr(((PCACHE_SCHEMA_BY_DISPLAYNAME)(FirstStruct))->displayname, ((PCACHE_SCHEMA_BY_DISPLAYNAME)(SecondStruct))->displayname);
}

static void _Function_class_(const PFN_CACHE_ENTRY_DESTROY_CALLBACK) pfnObjEntryDestroy(
	_In_ const PCACHE ppCache,
	_In_ const PVOID pvEntry
) {
	PIMPORTED_OBJECT impObj = NULL;

	if (ppCache == gs_CacheObjectBySid) {
		impObj = ((PCACHE_OBJECT_BY_SID)pvEntry)->object;
	}
	else if (ppCache == gs_CacheObjectByDn) {
		impObj = ((PCACHE_OBJECT_BY_DN)pvEntry)->object;
	}
	else {
		return;
	}
	
	IMPORTED_DECREF(impObj);
	if (IMPORTED_REFCOUNT(impObj) == 0) {
		UtilsHeapFreeHelper(gs_hHeapCache, impObj->imported.dn);
		UtilsHeapFreeHelper(gs_hHeapCache, impObj->imported.objectClassesNames);
		UtilsHeapFreeHelper(gs_hHeapCache, impObj->imported.mail);
		UtilsHeapFreeHelper(gs_hHeapCache, impObj);
	}
}

static void _Function_class_(const PFN_CACHE_ENTRY_DESTROY_CALLBACK) pfnSchEntryDestroy(
	_In_ const PCACHE ppCache,
	_In_ const PVOID pvEntry
) {
	PIMPORTED_SCHEMA impSch = NULL;

	if (ppCache == gs_CacheSchemaByGuid) {
		impSch = ((PCACHE_SCHEMA_BY_GUID)pvEntry)->schema;
	}
	else if (ppCache == gs_CacheSchemaByDisplayName) {
		impSch = ((PCACHE_SCHEMA_BY_DISPLAYNAME)pvEntry)->schema;
	}
	else {
		return;
	}

	IMPORTED_DECREF(impSch);
	if (IMPORTED_REFCOUNT(impSch) == 0) {
		UtilsHeapFreeHelper(gs_hHeapCache, impSch->imported.dn);
		UtilsHeapFreeHelper(gs_hHeapCache, impSch->imported.defaultSecurityDescriptor);
		UtilsHeapFreeHelper(gs_hHeapCache, impSch->imported.lDAPDisplayName);
		LocalFree(impSch->computed.defaultSD);
		UtilsHeapFreeHelper(gs_hHeapCache, impSch);
	}
}
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
BOOL CachesInitialize(
    ) {
	BOOL bResult = FALSE;

	bResult = UtilsHeapCreate(&gs_hHeapCache, CACHE_HEAP_NAME, NULL);
	if (API_FAILED(bResult)) {
		return ERROR_VALUE;
	}
	// Experimental use of Heap resource optimization (Win8.1+)
	/*
	HEAP_OPTIMIZE_RESOURCES_INFORMATION heapInformation = {
		.Version = HEAP_OPTIMIZE_RESOURCES_CURRENT_VERSION,
		.Flags = 0
	};
	HeapSetInformation(gs_hHeapCache->hHeap, HeapOptimizeResources, &heapInformation, sizeof(HEAP_OPTIMIZE_RESOURCES_INFORMATION));
	*/

	CacheCreate(&gs_CacheObjectBySid, _T("CacheObjectBySid"), AvlCompareObjectSid);
	CacheCreate(&gs_CacheObjectByDn, _T("CacheObjectByDn"), AvlCompareObjectDn);
	CacheCreate(&gs_CacheSchemaByGuid, _T("CacheSchemaByGuid"), AvlCompareSchemaGuid);
	CacheCreate(&gs_CacheSchemaByDisplayName, _T("CacheSchemaByDisplayName"), AvlCompareSchemaDisplayName);
    return TRUE;
}

void CachesDestroy(
    ) {
	BOOL bResult = FALSE;

	CacheDestroy(&gs_CacheObjectBySid, pfnObjEntryDestroy, sizeof(CACHE_OBJECT_BY_SID));
	CacheDestroy(&gs_CacheObjectByDn, pfnObjEntryDestroy, sizeof(CACHE_OBJECT_BY_DN));
	CacheDestroy(&gs_CacheSchemaByGuid, pfnSchEntryDestroy, sizeof(CACHE_SCHEMA_BY_GUID));
	CacheDestroy(&gs_CacheSchemaByDisplayName, pfnSchEntryDestroy, sizeof(CACHE_SCHEMA_BY_DISPLAYNAME));

	bResult = UtilsHeapDestroy(&gs_hHeapCache);
	if (API_FAILED(bResult)) {
		LOG(Err, _T("Failed to destroy heap: <%u>"), GetLastError());
	}
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

DWORD CacheSchemaByDisplayNameCount(
	) {
	return gs_SchemaByDisplayNameCount;
}

void CacheInsertObject(
    _In_ PIMPORTED_OBJECT obj
    ) {
    PIMPORTED_OBJECT object = (PIMPORTED_OBJECT)UtilsHeapAllocHelper(gs_hHeapCache, sizeof(IMPORTED_OBJECT));
    IMPORTED_FIELD_CST_CPY(object, obj, computed.sidLength);
    IMPORTED_FIELD_CST_CPY(object, obj, computed.number);
	IMPORTED_FIELD_CST_CPY(object, obj, computed.objectClassCount);
	IMPORTED_FIELD_CST_CPY(object, obj, imported.adminCount);
    IMPORTED_FIELD_BUF_CPY(object, obj, imported.sid, obj->computed.sidLength);
    IMPORTED_FIELD_DUP_CPY(object, obj, imported.dn);
	if(obj->imported.mail)
		IMPORTED_FIELD_DUP_CPY(object, obj, imported.mail);
	IMPORTED_FIELD_ARR_CPY(object, obj, imported.objectClassesNames, obj->computed.objectClassCount);

    //
    // Cache Object-By-Sid
    //
    if (object->computed.sidLength && !SID_EMPTY(&object->imported.sid, object->computed.sidLength)) {
        CACHE_OBJECT_BY_SID cacheEntry = { 0 };
        PCACHE_OBJECT_BY_SID inserted = NULL;
        BOOL newElement = FALSE;

        RtlCopyMemory(cacheEntry.sid, &object->imported.sid, object->computed.sidLength);
        cacheEntry.object = object;
		CacheEntryInsert(gs_CacheObjectBySid, (PVOID)&cacheEntry, sizeof(CACHE_OBJECT_BY_SID),&inserted, &newElement);
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
        BOOL newElement = FALSE;

        cacheEntry.dn = object->imported.dn;
        cacheEntry.object = object;

		CacheEntryInsert(gs_CacheObjectByDn, (PVOID)&cacheEntry, sizeof(CACHE_OBJECT_BY_DN), &inserted, &newElement);
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
}

void CacheInsertSchema(
    _In_ PIMPORTED_SCHEMA sch
    ) {
    PIMPORTED_SCHEMA schema = (PIMPORTED_SCHEMA)UtilsHeapAllocHelper(gs_hHeapCache, sizeof(IMPORTED_SCHEMA));
    IMPORTED_FIELD_CST_CPY(schema, sch, computed.number);
    IMPORTED_FIELD_BUF_CPY(schema, sch, imported.schemaIDGUID, sizeof(GUID));
    IMPORTED_FIELD_DUP_CPY(schema, sch, imported.dn);
	if (!STR_EMPTY(sch->imported.lDAPDisplayName))
	IMPORTED_FIELD_DUP_CPY(schema, sch, imported.lDAPDisplayName);
	if (!STR_EMPTY(sch->imported.defaultSecurityDescriptor))
		IMPORTED_FIELD_DUP_CPY(schema, sch, imported.defaultSecurityDescriptor);

    //
    // Cache Schema-By-Guid
    //
    if (!GUID_EMPTY(&schema->imported.schemaIDGUID)) {
        CACHE_SCHEMA_BY_GUID cacheEntry = { 0 };
        PCACHE_SCHEMA_BY_GUID inserted = NULL;
        BOOL newElement = FALSE;

        cacheEntry.guid = schema->imported.schemaIDGUID;
        cacheEntry.schema = schema;
		CacheEntryInsert(gs_CacheSchemaByGuid, (PVOID)&cacheEntry, sizeof(CACHE_SCHEMA_BY_GUID), &inserted, &newElement);
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
	// Cache Schema-By-DisplayName
	//
	if (!GUID_EMPTY(&schema->imported.lDAPDisplayName)) {
		CACHE_SCHEMA_BY_DISPLAYNAME cacheEntry = { 0 };
		PCACHE_SCHEMA_BY_DISPLAYNAME inserted = NULL;
		BOOL newElement = FALSE;

		cacheEntry.displayname = schema->imported.lDAPDisplayName;
		cacheEntry.schema = schema;

		CacheEntryInsert(gs_CacheSchemaByDisplayName, (PVOID)&cacheEntry, sizeof(CACHE_SCHEMA_BY_DISPLAYNAME), &inserted, &newElement);
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
}

PIMPORTED_OBJECT CacheLookupObjectBySid(
    _In_ PSID sid
    ) {
    CACHE_OBJECT_BY_SID searched = { 0 };
    PCACHE_OBJECT_BY_SID returned = NULL;

    RtlCopyMemory(&searched.sid, sid, GetLengthSid(sid));
	CacheEntryLookup(gs_CacheObjectBySid, (PVOID)&searched, &returned);
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
	CacheEntryLookup(gs_CacheObjectByDn, (PVOID)&searched, &returned);
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
	CacheEntryLookup(gs_CacheSchemaByGuid, (PVOID)&searched, &returned);
    if (!returned) {
        LOG(All, _T("cannot find schema-by-guid entry for <%s>"), "TODO");
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
	CacheEntryLookup(gs_CacheSchemaByDisplayName, (PVOID)&searched, &returned);
	if (!returned) {
		LOG(Dbg, _T("cannot find schema-by-displayname entry for <%s>"), displayname);
		return NULL;
	}
	return returned->schema;
}

LPTSTR CacheGetDomainDn(
    ) {
	BOOL bResult = FALSE;
    LPTSTR domain = NULL;
	PSID sid = NULL;
	// To lookup any one object
	LPTSTR stringSid = _T("S-1-5-32-544");
	PIMPORTED_OBJECT obj = NULL;

	bResult = ConvertStringSidToSid(stringSid, &sid);	
	if (!bResult || !sid) {
		LOG(Err, _T("Could not convert string sid to sid"));
		return NULL;
	}
	obj = CacheLookupObjectBySid(sid);
	LocalFree(sid);
	if (!obj) {
		LOG(Dbg, _T("cannot find object-by-sid entry for <%s>"), _T("S-1-5-32-544"));
		return NULL;
	}

    domain = _tcsstr(obj->imported.dn, _T("dc="));
	if (domain) {
		return domain;
	}
    return NULL;
}

GUID *CacheGetAdmPwdGuid(
) {
	PIMPORTED_SCHEMA sch = NULL;

	sch = CacheLookupSchemaByDisplayName(_T("ms-mcs-admpwd"));
	if (!sch) {
		LOG(Dbg, _T("cannot find schema by displayname entry for <%s>"), _T("ms-mcs-admpwd"));
		return NULL;
	}

	return &(sch->imported.schemaIDGUID);
}
