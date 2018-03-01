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
static LOG_LEVEL gs_debugLevel = Succ;
static HANDLE gs_hLogFile = INVALID_HANDLE_VALUE;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
struct _WELLKNOWN_BIGRAMS wellKnownBigrams[] = {
	{ _T(";RO)"),_T("-498") },
	{ _T(";LA)"),_T("-500") },
	{ _T(";LG)"),_T("-501") },
	{ _T(";DA)"),_T("-512") },
	{ _T(";DU)"),_T("-513") },
	{ _T(";DG)"),_T("-514") },
	{ _T(";DC)"),_T("-515") },
	{ _T(";DD)"),_T("-516") },
	{ _T(";CA)"),_T("-517") },
	{ _T(";SA)"),_T("-518") },
	{ _T(";EA)"),_T("-519") },
	{ _T(";PA)"),_T("-520") },
	{ _T(";CN)"),_T("-522") },
	{ _T(";AP)"),_T("-525") },
	{ _T(";RS)"),_T("-553") },
};


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
        LOG(Err, _T("Failed to resolve object <%s> for ace <%d>"), ace->imported.objectDn, ace->computed.number);
    }
    else {
        for (i = 0; i < object->computed.objectClassCount; i++) {
            schemaClass = GetSchemaByDisplayName(object->imported.objectClassesNames[i]);
            if (!schemaClass) {
                LOG(Err, _T("Failed to get class %s of object <%s> (ace <%u>)"), object->imported.objectClassesNames[i], object->imported.dn, ace->computed.number);
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

BOOL isObjectTypeClass(
	_In_ PIMPORTED_ACE ace
	) {
	DWORD dwFlags = 0;
	PIMPORTED_OBJECT objectSchemaObjectType = NULL;
	GUID * objectType = NULL;
	PIMPORTED_SCHEMA schemaObjectType = NULL;
	//
	// No ObjectType GUID ?
	//
	dwFlags = IS_OBJECT_ACE(ace->imported.raw) ? GetObjectFlags(ace) : 0;
	if ((dwFlags & ACE_OBJECT_TYPE_PRESENT) == 0)
		return FALSE;

	objectType = GetObjectTypeAce(ace);
	schemaObjectType = GetSchemaByGuid(objectType);
	if (!schemaObjectType) {
		LOG(Dbg, _T("ObjectType GUID does not represent anything from the schema. object <%s> for ACE <%u>."), ace->imported.objectDn, ace->computed.number);
		return FALSE;
	}
	objectSchemaObjectType = ResolverGetSchemaObject(schemaObjectType);
	if (!objectSchemaObjectType || objectSchemaObjectType->computed.objectClassCount != 2 || _tcscmp(objectSchemaObjectType->imported.objectClassesNames[1], _T("classschema"))) {
		LOG(Dbg, _T("ObjectType GUID does not represent a class. object <%s> for ACE <%u>."), ace->imported.objectDn, ace->computed.number);
		return FALSE;
	}
	else {
		LOG(Dbg, _T("Ace <%u> objectType GUID is a class. object <%s> (<%s>)"), ace->computed.number, ace->imported.objectDn, schemaObjectType->imported.lDAPDisplayName);
		return TRUE;
	}
}


BOOL isObjectTypeClassMatching(
	_In_ PIMPORTED_ACE ace
	) {
	DWORD dwFlags = 0;
	DWORD i = 0;
	//
	// We return FALSE for any non-matching conditions
	//
	dwFlags = IS_OBJECT_ACE(ace->imported.raw) ? GetObjectFlags(ace) : 0;
	if ((dwFlags & ACE_OBJECT_TYPE_PRESENT) == 0)
		return FALSE;
	//
	// We have to check the objectType, to see if the ACE actually applies to this object.
	// For this, we have to check if the objectType effectively represents a CLASS and then one of the right ones.
	// A few considerations regarding Object Type ACE:
	// - The OT Guid should be an Extended Right if Mask 0x100 bit is set, a Property if 0x20 bit is set and a Class in other cases;
	// - In non-consistent cases where those 3 situations are mixed, if the guid is of a Class; it is always evaluated, 
	//whereas if it is of a Property or of an Extended right, the corresponding bit must be set in the Mask or the guid is ignored;
	// - In case of mixing create/delete child and other bits in the Mask, the child is checked for create/delete child as per MSDN
	//and the current object is checked for every other right;
	// - Note that Object Type guid can match any class of the object to determine if ACE applies, whereas InheritedObjectType guid must match 
	//the final one (ObjectCategory) to determine if inherit_only flag is unset.
	PIMPORTED_OBJECT object = ResolverGetAceObject(ace);
	GUID * objectType = GetObjectTypeAce(ace);
	PIMPORTED_SCHEMA schemaObjectType = GetSchemaByGuid(objectType);

	if (!object) {
		LOG(Dbg, _T("Cannot lookup object <%s> to verify ACE filtering for ACE <%u>. Keep by default."), ace->imported.objectDn, ace->computed.number);
		return FALSE;
	}
	if (!schemaObjectType) {
		// ObjectType GUID does not represent anything from the schema, this is not ACE filtering so we keep it.
		return FALSE;
	}
	if (isObjectTypeClass(ace)) {
		for (i = 0; i < object->computed.objectClassCount; i++) {
			if (_tcscmp(object->imported.objectClassesNames[i],schemaObjectType->imported.lDAPDisplayName) == 0) {
				LOG(Dbg, _T("Ace <%u> objectType GUID is a matching class. (<%s>:  %s)"), ace->computed.number, ace->imported.objectDn, schemaObjectType->imported.lDAPDisplayName);
				return TRUE; // found a match
			}
		}
		LOG(Dbg, _T("Ace <%u> objectType GUID is a non-matching class (<%s>: %s)."), ace->computed.number, ace->imported.objectDn, schemaObjectType->imported.lDAPDisplayName);
		return FALSE;
	}
	else {
		return FALSE;
	}
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
		// Either resolved or computed...
        PIMPORTED_OBJECT obj = ResolverGetAceTrustee(ace);
        if (obj) {
			if (_tcsstr(obj->imported.dn, _T("cn=self,cn=wellknown security principals,cn=configuration"))) {
				ace->computed.trusteeStr = ace->imported.objectDn;
			}
			else {
				ace->computed.trusteeStr = obj->imported.dn;
			}
        }
        else {
			// this will leak, should be infrequent though
            ConvertSidToStringSid(GetTrustee(ace), &ace->computed.trusteeStr);
			CharLower(ace->computed.trusteeStr);
        }
    }

    return ace->computed.trusteeStr;
}

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

LPTSTR ResolverGetAceObjectMail(
	_In_ PIMPORTED_ACE ace
) {
	if (!ace->resolved.mail) {
		PIMPORTED_OBJECT obj = ResolverGetAceObject(ace);
		if (obj) {
			ace->resolved.mail = obj->imported.mail;
		}
		else {
			LOG(Dbg, _T("Cannot resolve object <%s> for ace <%u>"), ace->imported.objectDn, ace->computed.number);
		}
	}
	
	return NULL_IF_BAD(ace->resolved.mail);
}

PIMPORTED_OBJECT ResolverGetSchemaObject(
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

PSECURITY_DESCRIPTOR ResolverGetSchemaDefaultSD(
    _In_ PIMPORTED_SCHEMA sch
    ) {
	BOOL bResult = FALSE;
	LPTSTR backslashPos = NULL;
	LPTSTR destinationSddl = NULL;
	LPTSTR sourceSddl = NULL;
	size_t szSddl = 0;
	LPTSTR currentSID = NULL;
	LPTSTR current = NULL;
	LPTSTR domainStringSid = NULL;
	PIMPORTED_OBJECT objDomain = NULL;
	DWORD i = 0;
	HANDLE processHeap = NULL;

	processHeap = GetProcessHeap();
    if (!sch->computed.defaultSD && sch->imported.defaultSecurityDescriptor) {
        sch->computed.defaultSD = BAD_POINTER;
        //
        // Replace known bigrams that have a domain-depending SID
		// Otherwise ConvertStringSecurityDescriptorToSecurityDescriptor fails on non-domain computers
        // because they don't know the mapping of well-known bigrams used in SDDL (DA -> Domain Admins, etc.)
        //
		sourceSddl = UtilsHeapStrDupHelper((PUTILS_HEAP)&processHeap, sch->imported.defaultSecurityDescriptor);
		szSddl = _tcslen(sourceSddl);
		CharUpper(sourceSddl);
		backslashPos = _tcschr(sourceSddl, _T('\\'));
		while (backslashPos) {
			(TCHAR)*backslashPos = '\x00\x00';
			_tcscat_s(sourceSddl, szSddl, backslashPos + 1);
			backslashPos = _tcschr(sourceSddl, _T('\\'));
		}
        objDomain = GetObjectByDn(GetDomainDn());
		// Do not lowercase the sids here, we need them as-is for conversion to binary sid
		ConvertSidToStringSid(objDomain->imported.sid, &domainStringSid);
		currentSID = UtilsHeapAllocStrHelper((PUTILS_HEAP)&processHeap,MAX_SID_SIZE);
		for (i = 0; i < ARRAY_COUNT(wellKnownBigrams); i++) {
			RtlZeroMemory(currentSID, MAX_SID_SIZE * sizeof(TCHAR));
			_tcscat_s(currentSID, MAX_SID_SIZE, domainStringSid);
			_tcscat_s(currentSID, MAX_SID_SIZE, wellKnownBigrams[i].rid);

			current = _tcsstr(sourceSddl, wellKnownBigrams[i].bigram);
			while (current) {
				szSddl += _tcslen(currentSID) - 2;
				destinationSddl = UtilsHeapAllocHelper((PUTILS_HEAP)&processHeap, (DWORD)(szSddl * sizeof(TCHAR)));
				memcpy_s(destinationSddl, szSddl * sizeof(TCHAR), sourceSddl, (current - sourceSddl + 1) * sizeof(TCHAR)); //also copy the ;
				_tcscat_s(destinationSddl, szSddl, currentSID);
				_tcscat_s(destinationSddl, szSddl, current + 3);
				UtilsHeapFreeHelper((PUTILS_HEAP)&processHeap, sourceSddl);
				sourceSddl = UtilsHeapStrDupHelper((PUTILS_HEAP)&processHeap, destinationSddl);
				UtilsHeapFreeHelper((PUTILS_HEAP)&processHeap, destinationSddl);
				current = _tcsstr(sourceSddl, wellKnownBigrams[i].bigram);
			}
		}
		PSECURITY_DESCRIPTOR psd = NULL;
		bResult = ConvertStringSecurityDescriptorToSecurityDescriptor(sourceSddl, SDDL_REVISION_1, &psd, NULL);
        //bResult = ConvertStringSecurityDescriptorToSecurityDescriptor(sourceSddl, SDDL_REVISION_1, &sch->computed.defaultSD, NULL);
        if (!bResult) {
            LOG(Err, _T("Failed to convert defaultSD of class <%s> : <%u>"), sch->imported.dn, GetLastError());
            sch->computed.defaultSD = BAD_POINTER;
        }
        else {
            LOG(Dbg, _T("Successfully converted defaultSD of class <%s>"), sch->imported.dn);
			sch->computed.defaultSD = UtilsHeapMemDupHelper((PUTILS_HEAP)&processHeap, psd, GetSecurityDescriptorLength(psd));
        }
		UtilsHeapFreeHelper((PUTILS_HEAP)&processHeap, currentSID);
		UtilsHeapFreeHelper((PUTILS_HEAP)&processHeap, sourceSddl);
		LocalFree(domainStringSid);
		LocalFree(psd);
    }
    return NULL_IF_BAD(sch->computed.defaultSD);
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

PIMPORTED_SCHEMA GetSchemaByDisplayName(
	_In_ LPTSTR displayname
	) {
	if (!gs_SchemaCacheActivated) {
		FATAL(_T("Using schema cache (DisplayName) while it has not been activated (plugin missing requirement?)"));
	}

	return CacheLookupSchemaByDisplayName(displayname);
}

LPTSTR GetDomainDn(
    ) {
    if (!gs_ObjectCacheActivated) {
        FATAL(_T("Using object cache (Domain) while it has not been activated (plugin missing requirement?)"));
    }

    return CacheGetDomainDn();
}

GUID *GetAdmPwdGuid(
) {
	if (!gs_SchemaCacheActivated) {
		FATAL(_T("Using schema cache (Domain) while it has not been activated (plugin missing requirement?)"));
	}

	return CacheGetAdmPwdGuid();
}

LPTSTR GetAceRelationStr(
    _In_ ACE_RELATION rel
    ) {
    if (rel < 0 || rel >= ACE_REL_COUNT) {
        FATAL(_T("Unknown ace relation <%u>"), rel);
    }

    return gc_AceRelations[rel];
}

// Function taken from another project by A.Bordes.
BOOL IsAceInSd(
	_In_ PACE_HEADER pHeaderAceToCheck,
	_In_ PSECURITY_DESCRIPTOR pSd
) {
	BOOL bResult, bDaclPresent, bDaclDefaulted;
	ACL_SIZE_INFORMATION AclSizeInfo;
	PACL pDacl;
	PACE_HEADER pAceHeader;

	// S'il n'y a pas de descripteur de sécurité, on renvoie tourjous FALSE
	if (!pSd)
		return FALSE;

	bResult = GetSecurityDescriptorDacl(pSd, &bDaclPresent, &pDacl, &bDaclDefaulted);
	if (!bResult || !bDaclPresent)
		return FALSE;

	bResult = GetAclInformation(pDacl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation);
	if (!bResult)
		return FALSE;

	// On récupère les informations
	for (DWORD i = 0; i < AclSizeInfo.AceCount; i++)
	{
		// On récupère l'ACE
		bResult = GetAce(pDacl, i, (LPVOID*)&pAceHeader);
		if (!bResult)
			return FALSE;

		if (pHeaderAceToCheck->AceType == pAceHeader->AceType)
		{
			if ((pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) || (pAceHeader->AceType == ACCESS_DENIED_ACE_TYPE))
			{
				PACCESS_ALLOWED_ACE pAce, pAceToCheck;
				pAce = (PACCESS_ALLOWED_ACE)pAceHeader;
				pAceToCheck = (PACCESS_ALLOWED_ACE)pHeaderAceToCheck;

				if ((pAce->Mask == pAceToCheck->Mask)
					&& (EqualSid(&pAce->SidStart, &pAceToCheck->SidStart)))
				{
					return TRUE;
				}
			}
			else if ((pAceHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) || (pAceHeader->AceType == ACCESS_DENIED_OBJECT_ACE_TYPE))
			{
				PACCESS_ALLOWED_OBJECT_ACE pAce, pAceToCheck;
				pAce = (PACCESS_ALLOWED_OBJECT_ACE)pAceHeader;
				pAceToCheck = (PACCESS_ALLOWED_OBJECT_ACE)pHeaderAceToCheck;

				PSID pSidToCheck, pSidAce;

				if ((pAce->Flags == pAceToCheck->Flags) && (pAce->Mask == pAceToCheck->Mask))
				{
					if (pAce->Flags == 0)
					{
						// MSDN (SidStart) : The offset of this member can vary.
						// If the Flags member is zero, the SidStart member starts at the offset specified by the ObjectType member.
						pSidToCheck = &pAceToCheck->ObjectType;
						pSidAce = &pAce->ObjectType;

						if (EqualSid(pSidToCheck, pSidAce))
							return TRUE;
					}
					else if ((pAce->Flags == ACE_OBJECT_TYPE_PRESENT) || (pAce->Flags == ACE_INHERITED_OBJECT_TYPE_PRESENT))
					{
						// If Flags contains only one flag (either ACE_OBJECT_TYPE_PRESENT
						// or ACE_INHERITED_OBJECT_TYPE_PRESENT), the SidStart member starts at
						// the offset specified by the InheritedObjectType member.
						pSidToCheck = &pAceToCheck->InheritedObjectType;
						pSidAce = &pAce->InheritedObjectType;

						if ((EqualSid(pSidToCheck, pSidAce)) &&
							(GUID_EQ(&pAceToCheck->ObjectType, &pAce->ObjectType)))
							return TRUE;
					}
					else
					{
						pSidToCheck = &pAceToCheck->SidStart;
						pSidAce = &pAce->SidStart;

						if ((EqualSid(pSidToCheck, pSidAce)) &&
							(GUID_EQ(&pAceToCheck->ObjectType, &pAce->ObjectType)) &&
							(GUID_EQ(&pAceToCheck->InheritedObjectType, &pAce->InheritedObjectType)))
							return TRUE;
					}
				}
			}
			else
			{
				FATAL_FCT(_T("Unknown ACE type <%u>"), pAceHeader->AceType);
			}
		}
	}

	return FALSE;
}
