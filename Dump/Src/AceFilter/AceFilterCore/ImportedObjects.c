/******************************************************************************\
\******************************************************************************/


/* --- INCLUDES ------------------------------------------------------------- */
#include "ImportedObjects.h"

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static SECURITY_DESCRIPTOR gs_AdminSdHolderSD = { 0 };
static BYTE gs_AdminSdHolderACL[ACL_ADMIN_SD_HOLDER_SIZE] = { 0 };
static PSECURITY_DESCRIPTOR gs_pAdminSdHolderSD = NULL;
static BOOL gs_ObjectCacheActivated = FALSE;
static BOOL gs_SchemaCacheActivated = FALSE;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
ACCESS_MASK GetAccessMask(
    _In_ PIMPORTED_ACE impace
    ) {
    DWORD val = 0;
    PACE_HEADER ace = impace->imported.raw;

    switch (ace->AceType) {
        HELPER_SWITCH_ACE_TYPE_GET_ALL(val, ace, Mask, /*NONE*/);
    default:
        FATAL_FCT(_T("Unsupported ACE type <%#x>"), ace->AceType);
    }

    return val;
}

PSID GetTrustee(
    _In_ PIMPORTED_ACE impace
    ) {
    PSID sid = NULL;
    PACE_HEADER ace = impace->imported.raw;

    if (IS_OBJECT_ACE(ace)) {
        DWORD flags = GetObjectFlags(impace);
        if (flags == 0) {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(sid, ace, ObjectType, (PSID)&);
            }
        }
        else if ((flags == ACE_OBJECT_TYPE_PRESENT) || (flags == ACE_INHERITED_OBJECT_TYPE_PRESENT)) {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(sid, ace, InheritedObjectType, (PSID)&);
            }
        }
        else {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(sid, ace, SidStart, (PSID)&);
            }
        }
    }
    else {
        switch (ace->AceType) {
            HELPER_SWITCH_ACE_TYPE_GET_ALL(sid, ace, SidStart, (PSID)&);
        default:
            FATAL_FCT(_T("Unsupported ACE type <%#x>"), ace->AceType);
        }
    }

    return sid;
}

DWORD GetObjectFlags(
    _In_ PIMPORTED_ACE impace
    ) {
    DWORD flags = 0;
    PACE_HEADER ace = impace->imported.raw;

    switch (ace->AceType) {
        HELPER_SWITCH_ACE_TYPE_GET_OBJECT(flags, ace, Flags, /*NONE*/);
    default:
        FATAL_FCT(_T("Unsupported ACE_OBJECT type <%#x>"), ace->AceType);
    }

    return flags;
}

GUID *GetObjectTypeAce(
    _In_ PIMPORTED_ACE impace
    ) {
    GUID * objectType = NULL;
    PACE_HEADER ace = impace->imported.raw;

    if (IS_OBJECT_ACE(ace)) {
        DWORD flags = GetObjectFlags(impace);

        if (flags & ACE_OBJECT_TYPE_PRESENT) {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(objectType, ace, ObjectType, &);
            }
        }
    }

    return objectType;
}

GUID *GetInheritedObjectTypeAce(
    _In_ PIMPORTED_ACE impace
    ) {
    GUID * inheritedObjectType = NULL;
    PACE_HEADER ace = impace->imported.raw;

    if (IS_OBJECT_ACE(ace)) {
        DWORD flags = GetObjectFlags(impace);
        if ((flags & ACE_OBJECT_TYPE_PRESENT) == 0) {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(inheritedObjectType, ace, ObjectType, (PSID)&);
            }
        }
        else {
            switch (ace->AceType) {
                HELPER_SWITCH_ACE_TYPE_GET_OBJECT(inheritedObjectType, ace, InheritedObjectType, (PSID)&);
            }
        }
    }

    return inheritedObjectType;
}

void InitAdminSdHolderSd(
    ) {
    BOOL bResult = FALSE;

    if (!gs_pAdminSdHolderSD) {

        bResult = InitializeSecurityDescriptor(&gs_AdminSdHolderSD, SECURITY_DESCRIPTOR_REVISION);
        if (!bResult) {
            FATAL(_T("Failed to initilialize AdminSDHolder's SD : <%u>"), GetLastError());
        }

        bResult = InitializeAcl((PACL)&gs_AdminSdHolderACL, ACL_ADMIN_SD_HOLDER_SIZE, ACL_REVISION_DS);
        if (!bResult) {
            FATAL(_T("Failed to initilialize AdminSDHolder's ACL : <%u>"), GetLastError());
        }

        bResult = SetSecurityDescriptorDacl(&gs_AdminSdHolderSD, TRUE, (PACL)&gs_AdminSdHolderACL, FALSE);
        if (!bResult) {
            FATAL(_T("Failed to affect AdminSDHolder's ACL : <%u>"), GetLastError());
        }

        gs_pAdminSdHolderSD = &gs_AdminSdHolderSD;
    }
}

void AddAceToAdminSdHolderSd(
    _In_ PIMPORTED_ACE ace
    ) {
    BOOL bResult = FALSE;

    if (gs_pAdminSdHolderSD) {
        bResult = AddAce((PACL)&gs_AdminSdHolderACL, ACL_REVISION_DS, MAXDWORD, ace->imported.raw, ace->imported.raw->AceSize);
        if (!bResult) {
            FATAL(_T("Failed to add ACE <%u> to AdminSDHolder's ACL : <%u>"), ace->computed.number, GetLastError());
        }
    }
}

BOOL IsInAdminSdHolder(
    _In_ PIMPORTED_ACE ace
    ) {

    if (gs_pAdminSdHolderSD == NULL) {
        FATAL(_T("Using AdminSdHolderSD while it has not been constructed (plugin missing requirement?)"));
    }

    return IsAceInSd(ace->imported.raw, gs_pAdminSdHolderSD);
}

BOOL IsInDefaultSd(
    _In_ PIMPORTED_ACE ace
    ) {
    DWORD i = 0;
    PSECURITY_DESCRIPTOR sd = NULL;
    PIMPORTED_SCHEMA schemaClass = NULL;
    PIMPORTED_OBJECT object = ResolverGetAceObject(ace);

    if (!object) {
        LOG(Err, _T("Failed to resolve object <%s> for ace <%s>"), ace->imported.objectDn, ace->computed.number);
    }
    else {
        for (i = 0; i < object->computed.objectClassCount; i++) {
            schemaClass = ResolverGetObjectObjectClass(object, i);
            if (!schemaClass) {
                LOG(Err, _T("Failed to get class <%#08x> of object <%s> (ace <%u>)"), object->imported.objectClassesIds[i], object->imported.dn, ace->computed.number);
            }
            else {
                sd = ResolverGetSchemaDefaultSD(schemaClass);
                if (sd && IsAceInSd(ace->imported.raw, sd)) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

void CacheActivateObjectCache(
    ) {
    gs_ObjectCacheActivated = TRUE;
}

void CacheActivateSchemaCache(
    ) {
    gs_SchemaCacheActivated = TRUE;
}

LPTSTR  ResolverGetAceTrusteeStr(
    _In_ PIMPORTED_ACE ace
    ){
    if (!ace->computed.trusteeStr) {
        PIMPORTED_OBJECT obj = ResolverGetAceTrustee(ace);
        if (obj) {
            ace->computed.trusteeStr = obj->imported.dn;
        }
        else {
            ConvertSidToStringSid(GetTrustee(ace), &ace->computed.trusteeStr);
        }
    }

    return ace->computed.trusteeStr;
}

/*
LPTSTR ResolverGetObjectPrimaryOwnerStr(
    _In_ PIMPORTED_OBJECT obj
    ){
    if (!obj->computed.primaryOwnerStr) {
        PIMPORTED_OBJECT ownerObj = ResolverGetObjectPrimaryOwner(obj);
        if (ownerObj) {
            obj->computed.primaryOwnerStr = ownerObj->imported.dn;
        }
        else {
            ConvertSidToStringSid(obj->imported.primaryOwner, &obj->computed.primaryOwnerStr);
        }
    }

    return obj->computed.primaryOwnerStr;
}
*/

/*
LPTSTR ResolverGetObjectPrimaryGroupStr(
    _In_ PIMPORTED_OBJECT obj
    ){
    if (!obj->computed.primaryGroupStr) {
        PIMPORTED_OBJECT groupObj = ResolverGetObjectPrimaryGroup(obj);
        if (groupObj) {
            obj->computed.primaryGroupStr = groupObj->imported.dn;
        }
        else {
            ConvertSidToStringSid(obj->imported.primaryGroup, &obj->computed.primaryGroupStr);
        }
    }

    return obj->computed.primaryGroupStr;
}
*/

PIMPORTED_OBJECT ResolverGetAceTrustee(
    _In_ PIMPORTED_ACE ace
    ) {
    if (!ace->resolved.trustee) {
        ace->resolved.trustee = GetObjectBySid(GetTrustee(ace));
        if (!ace->resolved.trustee) {
            LOG(Dbg, _T("Cannot resolve trustee SID for ace <%u>"), ace->computed.number);
            ace->resolved.trustee = BAD_POINTER;
        }
    }

    return NULL_IF_BAD(ace->resolved.trustee);
}

/*
PIMPORTED_OBJECT ResolverGetObjectPrimaryOwner(
    _In_ PIMPORTED_OBJECT obj
    ) {
    if (!obj->resolved.primaryOwner) {
        obj->resolved.primaryOwner = GetObjectBySid(obj->imported.primaryOwner);
        if (!obj->resolved.primaryOwner) {
            LOG(Dbg, _T("Cannot resolve primary-owner SID for object <%s> <%u>"), obj->imported.dn, obj->computed.number);
            obj->resolved.primaryOwner = BAD_POINTER;
        }
    }

    return NULL_IF_BAD(obj->resolved.primaryOwner);
}
*/

/*
PIMPORTED_OBJECT ResolverGetObjectPrimaryGroup(
    _In_ PIMPORTED_OBJECT obj
    ) {
    if (!obj->resolved.primaryGroup) {
        obj->resolved.primaryGroup = GetObjectBySid(obj->imported.primaryGroup);
        if (!obj->resolved.primaryGroup) {
            LOG(Dbg, _T("Cannot resolve primary-group SID for object <%s> <%u>"), obj->imported.dn, obj->computed.number);
            obj->resolved.primaryGroup = BAD_POINTER;
        }
    }

    return NULL_IF_BAD(obj->resolved.primaryGroup);
}
*/

PIMPORTED_OBJECT ResolverGetAceObject(
    _In_ PIMPORTED_ACE ace
    ) {
    if (!ace->resolved.object) {
        ace->resolved.object = GetObjectByDn(ace->imported.objectDn);
        if (!ace->resolved.object) {
            LOG(Dbg, _T("Cannot resolve object DN <%s> for ace <%u>"), ace->imported.objectDn, ace->computed.number);
            ace->resolved.object = BAD_POINTER;
        }
    }

    return NULL_IF_BAD(ace->resolved.object);
}

PSECURITY_DESCRIPTOR ResolverGetSchemaObject(
    _In_ PIMPORTED_SCHEMA sch
    ) {
    if (!sch->resolved.object) {
        sch->resolved.object = GetObjectByDn(sch->imported.dn);
        if (!sch->resolved.object) {
            LOG(Dbg, _T("Cannot resolve object DN <%s> for schema <%u>"), sch->imported.dn, sch->computed.number);
            sch->resolved.object = BAD_POINTER;
        }
    }

    return NULL_IF_BAD(sch->resolved.object);
}

PIMPORTED_SCHEMA ResolverGetObjectObjectClass(
    _In_ PIMPORTED_OBJECT obj,
    _In_ DWORD idx
    ) {
    if (idx >= obj->computed.objectClassCount) {
        return NULL;
    }
    if (!obj->resolved.objectClasses){
        obj->resolved.objectClasses = (PIMPORTED_SCHEMA*)LocalAllocCheckX(sizeof(PIMPORTED_SCHEMA)* obj->computed.objectClassCount);
    }
    if (!obj->resolved.objectClasses[idx]) {
        obj->resolved.objectClasses[idx] = GetSchemaByClassid(obj->imported.objectClassesIds[idx]);
        if (!obj->resolved.objectClasses[idx]) {
            obj->resolved.objectClasses[idx] = BAD_POINTER;
            LOG(Dbg, _T("Cannot resolve objectClass <%#08x> for object <%u>"), obj->imported.objectClassesIds[idx], obj->imported.dn);
        }
    }

    return NULL_IF_BAD(obj->resolved.objectClasses[idx]);
}

PSECURITY_DESCRIPTOR ResolverGetSchemaDefaultSD(
    _In_ PIMPORTED_SCHEMA sch
    ) {
    if (!sch->resolved.defaultSD) {
        sch->resolved.defaultSD = BAD_POINTER;

        //
        // TODO : removed for now, because ConvertStringSecurityDescriptorToSecurityDescriptor fails on non-domain computers
        //        because they don't know the mapping of well known bigrams used in SDDL (DA -> Domain Admins, etc.)
        //

        //BOOL bResult = FALSE;

        //bResult = ConvertStringSecurityDescriptorToSecurityDescriptor(sch->imported.defaultSecurityDescriptor, SDDL_REVISION_1, &sch->resolved.defaultSD, NULL);
        //if (!bResult) {
        //    LOG(Err, _T("Failed to convert defaultSD of class <%s> : <%u>"), sch->imported.dn, GetLastError());
        //    sch->resolved.defaultSD = BAD_POINTER;
        //}
        //else {
        //    LOG(Dbg, _T("Successfully converted defaultSD of class <%s>"), sch->imported.dn);
        //}
    }

    return NULL_IF_BAD(sch->resolved.defaultSD);
}


PIMPORTED_OBJECT GetObjectByDn(
    _In_ LPTSTR dn
    ) {
    if (!gs_ObjectCacheActivated) {
        FATAL(_T("Using object cache (DN) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheLookupObjectByDn(dn);
}

PIMPORTED_OBJECT GetObjectBySid(
    _In_ PSID sid
    ) {
    if (!gs_ObjectCacheActivated) {
        FATAL(_T("Using object cache (SID) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheLookupObjectBySid(sid);
}

PIMPORTED_SCHEMA GetSchemaByGuid(
    _In_ GUID * guid
    ) {
    if (!gs_SchemaCacheActivated) {
        FATAL(_T("Using schema cache (GUID) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheLookupSchemaByGuid(guid);
}

PIMPORTED_SCHEMA GetSchemaByClassid(
    _In_ DWORD classid
    ) {
    if (!gs_SchemaCacheActivated) {
        FATAL(_T("Using schema cache (ClassID) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheLookupSchemaByClassid(classid);
}

LPTSTR GetDomainDn(
    ) {
    if (!gs_ObjectCacheActivated) {
        FATAL(_T("Using object cache (Domain) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheGetDomainDn();
}

LPTSTR GetAceRelationStr(
    _In_ ACE_RELATION rel
    ) {
    if (rel < 0 || rel >= ACE_REL_COUNT) {
        FATAL(_T("Unknown ace relation <%u>"), rel);
    }

    return gc_AceRelations[rel];
}
