/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"

/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("ControlAd")
#define PLUGIN_KEYWORD      _T("CAD")
#define PLUGIN_DESCRIPTION  _T("Filters control ACE for AD objects");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_GUID_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
/*
cf. http://msdn.microsoft.com/en-us/library/windows/desktop/aa772285(v=vs.85).aspx
- ADS_RIGHT_DS_CREATE_CHILD
- ADS_RIGHT_DS_DELETE_CHILD
- ADS_RIGHT_DS_SELF
- ADS_RIGHT_DS_READ_PROP
- ADS_RIGHT_DS_WRITE_PROP
- ADS_RIGHT_DS_CONTROL_ACCESS
*/
#define OBJECT_TYPE_GUID_NEEDED(mask)  (mask & (ADS_RIGHT_DS_CREATE_CHILD|ADS_RIGHT_DS_DELETE_CHILD|ADS_RIGHT_DS_SELF|ADS_RIGHT_DS_READ_PROP|ADS_RIGHT_DS_WRITE_PROP|ADS_RIGHT_DS_CONTROL_ACCESS))

/* --- TYPES ---------------------------------------------------------------- */
typedef struct _CONTROL_GUID {
    GUID * guid;
    ACE_RELATION rel;
} CONTROL_GUID, *PCONTROL_GUID;

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
// Guids used later
static GUID gcs_GuidExtendedRightUserForceChangePassword = { 0x00299570, 0x246d, 0x11d0, { 0xa7, 0x68, 0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29 } }; // _T("00299570-246d-11d0-a768-00aa006e0529"); 
static GUID gcs_GuidExtendedRightDSReplicationGetChangesAll = { 0x1131f6ad, 0x9c07, 0x11d1, { 0xf7, 0x9f, 0x00, 0xc0, 0x4f, 0xc2, 0xdc, 0xd2 } }; // _T("1131f6ad-9c07-11d1-f79f-00c04fc2dcd2"); 
static GUID gcs_GuidPropertyMember = { 0xbf9679c0, 0x0de6, 0x11d0, { 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 } }; // _T("bf9679c0-0de6-11d0-a285-00aa003049e2"); 
static GUID gcs_GuidPropertyGpcFileSysPath = { 0xf30e3bc1, 0x9ff0, 0x11d0, { 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1} }; 
static GUID gcs_GuidPropertyGpLink = { 0xf30e3bbe, 0x9ff0, 0x11d1, { 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } }; // _T("f30e3bbe-9ff0-11d1-b603-0000f80367c1");
static GUID gcs_GuidPropertySetMembership = { 0xbc0ac240, 0x79a9, 0x11d0, { 0x90, 0x20, 0x00, 0xc0, 0x4f, 0xc2, 0xd4, 0xcf } }; // _T("bc0ac240-79a9-11d0-9020-00c04fc2d4cf");
static GUID gcs_GuidPropertySetPersonalInformation = { 0x77b5b886, 0x944a, 0x11d1, { 0xae, 0xbd, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } }; // _T("77b5b886-944a-11d1-aebd-0000f80367c1");
static GUID gcs_GuidValidatedWriteSelfMembership = { 0xbf9679c0, 0x0de6, 0x11d0, { 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 } }; // Same as gs_GuidPropertyMember. thanks AD. GUIDs are no longer "UNIQUE"

// Control properties
static CONTROL_GUID gcs_GuidsControlProperties[] = {
    { &gcs_GuidPropertyMember, WRITE_PROP_MEMBER },
    { &gcs_GuidPropertyGpLink, WRITE_PROP_GPLINK },
    { &gcs_GuidPropertyGpcFileSysPath, WRITE_PROP_GPC_FILE_SYS_PATH },
};

// Control property sets
static CONTROL_GUID gcs_GuidsControlPropertiesSets[] = {
    { &gcs_GuidPropertySetMembership, WRITE_PROPSET_MEMBERSHIP },
};

// Control extended rights
static CONTROL_GUID gcs_GuidsControlExtendedRights[] = {
    { &gcs_GuidExtendedRightUserForceChangePassword, EXT_RIGHT_FORCE_CHANGE_PWD },
    { &gcs_GuidExtendedRightDSReplicationGetChangesAll, EXT_RIGHT_REPLICATION_GET_CHANGES_ALL },
};

// Control validated writes
static CONTROL_GUID gcs_GuidsControlValidatedWrites[] = {
    { &gcs_GuidValidatedWriteSelfMembership, VAL_WRITE_SELF_MEMBERSHIP },
};

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Keeps ACE allowing control relations on AD objects. Currently checked control relations are :"));
    API_LOG(Bypass, _T("- Generic rights : GENERIC_ALL, GENERIC_WRITE"));
    API_LOG(Bypass, _T("- Standard rights : WRITE_DAC, WRITE_OWNER"));
    API_LOG(Bypass, _T("- Extended rights : All, User-Force-Change-Password, DS-Replication-Get-Changes-All"));
    API_LOG(Bypass, _T("- Validated writes : All, Self-Membership"));
    API_LOG(Bypass, _T("- Write properties : Member, GPLink"));
    API_LOG(Bypass, _T("- Write property-sets : Membership"));
    API_LOG(Bypass, _T("This filter set 'relations' on the ACE, and must be combined with the MSR writer for these relations to be outputed."));
}

BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    /*
    - Generic rights :
        - GENERIC_WRITE
        - GENERIC_ALL

    - Standards rights :
        - WRITE_DAC
        - WRITE_OWNER
        - WRITE_PROP :
    
    - Properties :
        - all (empty guid)
        - member
        - gPLink (TODO : cannot find a property set containing this attr)
    
    - Property sets :
        - membership (contains member)

    - Extended rights (ADS_RIGHT_DS_CONTROL_ACCESS) :
        - all (empty guid)
        - Reset password

    - Validated writes (ADS_RIGHT_DS_SELF) :
        - all (empty guid)
        - Add/remove self as member
    */

    DWORD dwAccessMask = 0;
    DWORD dwFlags = 0;
    DWORD i = 0;

    //
    // Only "*_ALLOWED_*" ace types can allow control
    //
    if (!IS_ALLOWED_ACE(ace->imported.raw))
        return FALSE;

    //
    // Get properties
    //
    dwAccessMask = api->Ace.GetAccessMask(ace);
    dwFlags = IS_OBJECT_ACE(ace->imported.raw) ? api->Ace.GetObjectFlags(ace) : 0;

    //
    // First, we have to check the objectType, to see if the ACE actually applies to this object.
    // For this, we have to check if the objectType effectively represents a CLASS, and that the
    // ACE does not only contains the CREATE/DELETE_CHILD rights (otherwise it is not a filter)
    //
    if (ace->imported.raw->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE || ace->imported.raw->AceType == ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE) {
        if (dwAccessMask != ADS_RIGHT_DS_CREATE_CHILD && dwAccessMask != ADS_RIGHT_DS_DELETE_CHILD && dwAccessMask != (ADS_RIGHT_DS_CREATE_CHILD | ADS_RIGHT_DS_DELETE_CHILD)) {
            if (dwFlags & ACE_OBJECT_TYPE_PRESENT) {
                PIMPORTED_OBJECT object = api->Resolver.ResolverGetAceObject(ace);
                GUID * objectType = api->Ace.GetObjectTypeAce(ace);
                PIMPORTED_SCHEMA schemaObjectType = api->Resolver.GetSchemaByGuid(objectType);

                if (!object) {
                    API_LOG(Err, _T("Cannot lookup object <%s> to verify ACE filtering for ACE <%u>. Skipping."), ace->imported.objectDn, ace->computed.number);
                    return FALSE;
                }

                if (!schemaObjectType) {
                    // Object class is not representing a "class", this is not ACE filtering
                }
                else {
                    PIMPORTED_OBJECT objectSchemaObjectType = api->Resolver.ResolverGetSchemaObject(schemaObjectType);
                    if (!objectSchemaObjectType || objectSchemaObjectType->computed.objectClassCount != 1 || objectSchemaObjectType->imported.objectClassesIds[i] != DW_CLASS_SCHEMA) {
                        // Object class is not representing a "class", this is not ACE filtering
                    }
                    else {
                        for (i = 0; i < object->computed.objectClassCount; i++) {
                            if (object->imported.objectClassesIds[i] == schemaObjectType->imported.governsID) {
                                API_LOG(Dbg, _T("Ace <%u> passed objectType filtering (<%s> of type <%#08x>)"), ace->computed.number, ace->imported.objectDn, object->imported.objectClassesIds[i]);
                                break; // found it
                            }
                        }

                        if (i == object->computed.objectClassCount) {
                            //
                            // If we arrived here, this means that :
                            //    - there is an ObjectType representing a "class" schema entry
                            //    - the AccessMask contains other rights than only CREATE/DELETE_CHILD
                            //    - the current object is not of the class pointed by the ObjectType
                            // => thus, the ACE does not apply (ACE ObjectType filtering)
                            //
                            API_LOG(Dbg, _T("Ace <%u> is filtered by its objectType <%#08x> (object <%s>)"), ace->computed.number, schemaObjectType->imported.governsID, ace->imported.objectDn);
                            return FALSE;
                        }
                    }
                }
            }
        }
    }


    //
    // Control case : Generic right GENERIC_WRITE
    //
    if (dwAccessMask & ADS_RIGHT_GENERIC_WRITE)
        SET_RELATION(ace, GEN_RIGHT_WRITE);

    //
    // Control case : Generic right GENERIC_ALL
    //
    if (dwAccessMask & ADS_RIGHT_GENERIC_ALL)
        SET_RELATION(ace, GEN_RIGHT_ALL);

    //
    // Control case : Standard right WRITE_DAC
    //
    if (dwAccessMask & ADS_RIGHT_WRITE_DAC)
        SET_RELATION(ace, STAND_RIGHT_WRITE_DAC);

    //
    // Control case : Standard right WRITE_OWNER
    //
    if (dwAccessMask & ADS_RIGHT_WRITE_OWNER)
        SET_RELATION(ace, STAND_RIGHT_WRITE_OWNER);

    // Control cases where ObjectType GUID is needed
    if (OBJECT_TYPE_GUID_NEEDED(dwAccessMask)) {

        if (!(dwFlags & ACE_OBJECT_TYPE_PRESENT)) {

            // extended right + empty guid == all extended rights
            if (dwAccessMask & ADS_RIGHT_DS_CONTROL_ACCESS)
                SET_RELATION(ace, EXT_RIGHT_ALL);

            // validated write + empty guid == all validated rights
            if (dwAccessMask & ADS_RIGHT_DS_SELF)
                SET_RELATION(ace, VAL_WRITE_ALL);

            // write prop + empty guid == write all properties
            if (dwAccessMask & ADS_RIGHT_DS_WRITE_PROP)
                SET_RELATION(ace, WRITE_PROP_ALL);

        }
        else {
            GUID * objectType = api->Ace.GetObjectTypeAce(ace);

            // Control cases with extended rights
            if (dwAccessMask & ADS_RIGHT_DS_CONTROL_ACCESS) {
                //
                // Control case : allow of control extended right
                // example : user change password
                //
                for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlExtendedRights); i++) {
                    if (GUID_EQ(objectType, gcs_GuidsControlExtendedRights[i].guid))
                        SET_RELATION(ace, gcs_GuidsControlExtendedRights[i].rel);
                }
            }

            // Control cases with validated writes
            if (dwAccessMask & ADS_RIGHT_DS_SELF) {
                //
                // Control case : allow of control validated write
                // example : self-membership
                //
                for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlValidatedWrites); i++) {
                    if (GUID_EQ(objectType, gcs_GuidsControlValidatedWrites[i].guid))
                        SET_RELATION(ace, gcs_GuidsControlValidatedWrites[i].rel);
                }
            }

            // Control cases with write properties and write property sets
            if (dwAccessMask & ADS_RIGHT_DS_WRITE_PROP) {
                //
                // Control case : WRITE_PROP of control property
                // example : "member" property
                //
                for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlProperties); i++) {
                    if (GUID_EQ(objectType, gcs_GuidsControlProperties[i].guid))
                        SET_RELATION(ace, gcs_GuidsControlProperties[i].rel);
                }

                //
                // Control case : WRITE_PROP of control property set
                // example : "membership" property set
                // (can actually be merged with control properties, but splitted in 2 arrays for readability)
                //
                for (i = 0; i < ARRAY_COUNT(gcs_GuidsControlPropertiesSets); i++) {
                    if (GUID_EQ(objectType, gcs_GuidsControlPropertiesSets[i].guid))
                        SET_RELATION(ace, gcs_GuidsControlPropertiesSets[i].rel);
                }
            }
        }
    } // OBJECT_TYPE_GUID_NEEDED(dwAccessMask))

    for (i = 0; i < ACE_REL_COUNT; i++) {
        if (HAS_RELATION(ace, i)) {
            return TRUE;
        }
    }

    return FALSE;
}