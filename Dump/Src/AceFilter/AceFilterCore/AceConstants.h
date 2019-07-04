/******************************************************************************\
    This file centralizes all possible keywords that can characterize the
    relation Object<->Trustee. An ACE can setup more than one relation.
\******************************************************************************/

#ifndef __ACE_RELATIONS_H__
#define __ACE_RELATIONS_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#pragma warning(disable:4201)
#include <Iads.h>
#pragma warning(default:4201)

/* --- DEFINES -------------------------------------------------------------- */
#define ACE_REL_COUNT                           (35)
#define SET_RELATION(ace, rel)                  BITMAP_SET_BIT((ace)->computed.relation, rel)
#define HAS_RELATION(ace, rel)                  BITMAP_GET_BIT((ace)->computed.relation, rel)

#define ACE_TYPES_VALUES_COUNT                  (25)
#define ACE_FLAGS_VALUES_COUNT                  (7)
#define ACE_OBJECT_FLAGS_VALUES_COUNT           (2)
#define ACE_FLAGS_VALUE_COUNT                   (4)
#define ACE_GENERIC_ACCESSMASK_VALUES_COUNT     (4)
#define ACE_STANDARD_ACCESSMASK_VALUES_COUNT    (6)
#define ACE_AD_ACCESSMASK_VALUES_COUNT          (9)
#define ACE_FS_ACCESSMASK_VALUES_COUNT          (8)

#define ACE_GENERIC_RIGHTS_MASK                 (0xF0000000)
#define ACE_STANDARD_RIGHTS_MASK                (STANDARD_RIGHTS_ALL|ACCESS_SYSTEM_SECURITY)
#define ACE_SPECIFIC_RIGHTS_MASK                (SPECIFIC_RIGHTS_ALL)

#define ADMIN_SD_HOLDER_PARTIAL_DN  _T("cn=adminsdholder,cn=system,dc=")


/* --- TYPES ---------------------------------------------------------------- */

typedef enum _ACE_RELATION {
    GEN_RIGHT_ALL = __COUNTER__,
    GEN_RIGHT_WRITE = __COUNTER__,
    GEN_RIGHT_READ = __COUNTER__,

    STAND_RIGHT_WRITE_DAC = __COUNTER__,
    STAND_RIGHT_WRITE_OWNER = __COUNTER__,

    EXT_RIGHT_ALL = __COUNTER__,
    EXT_RIGHT_FORCE_CHANGE_PWD = __COUNTER__,
    EXT_RIGHT_CERTIFICATE_ENROLLMENT = __COUNTER__,
    EXT_RIGHT_DOMAIN_ADMINISTER_SERVER = __COUNTER__,
    EXT_RIGHT_REPLICATION_GET_CHANGES_ALL = __COUNTER__,
    EXT_RIGHT_RECEIVE_AS = __COUNTER__,
    EXT_RIGHT_SEND_AS = __COUNTER__,
    EXT_RIGHT_CREATE_INBOUND_FOREST_TRUST = __COUNTER__,
    EXT_RIGHT_MIGRATE_SID_HISTORY = __COUNTER__,
    EXT_RIGHT_ADM_PWD = __COUNTER__,

    VAL_WRITE_ALL = __COUNTER__,
    VAL_WRITE_SELF_MEMBERSHIP = __COUNTER__,

    WRITE_PROP_ALL = __COUNTER__,
    WRITE_PROP_ALTSECID = __COUNTER__,
    WRITE_PROP_SPN = __COUNTER__,
    WRITE_PROP_MEMBER = __COUNTER__,
    WRITE_PROP_X509CERT = __COUNTER__,
    WRITE_PROP_GPLINK = __COUNTER__,
    WRITE_PROP_GPC_FILE_SYS_PATH = __COUNTER__,
    WRITE_PROPSET_MEMBERSHIP = __COUNTER__,
    WRITE_PROPSET_PUBINFO = __COUNTER__,

    FS_RIGHT_WRITEDATA_ADDFILE = __COUNTER__,
    FS_RIGHT_APPENDDATA_ADDSUBDIR = __COUNTER__,

    EXCH_CHANGEPERMISSION = __COUNTER__,
    EXCH_CHANGEOWNER = __COUNTER__,
    EXCH_FULLACCESS = __COUNTER__,
    EXCH_EXTERNALACCOUNT = __COUNTER__,

    MAPI_WRITESD = __COUNTER__,
    MAPI_WRITEOWNER = __COUNTER__,
    MAPI_READBODY = __COUNTER__,
} ACE_RELATION;

static_assert(__COUNTER__ == ACE_REL_COUNT, "inconsistent value for ACE_REL_COUNT");

/* --- VARIABLES ------------------------------------------------------------ */
extern const LPTSTR gc_AceRelations[ACE_REL_COUNT];
extern const NUMERIC_CONSTANT gc_AceTypeValues[ACE_TYPES_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceFlagsValues[ACE_FLAGS_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceGenericAccessMaskValues[ACE_GENERIC_ACCESSMASK_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceStandardAccessMaskValues[ACE_STANDARD_ACCESSMASK_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceSpecificAdAccessMaskValues[ACE_AD_ACCESSMASK_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceSpecificFsAccessMaskValues[ACE_FS_ACCESSMASK_VALUES_COUNT];
extern const NUMERIC_CONSTANT gc_AceObjectFlagsValues[ACE_OBJECT_FLAGS_VALUES_COUNT];

/* --- PROTOTYPES ----------------------------------------------------------- */
LPTSTR GetAceRelationStr(
    _In_ ACE_RELATION rel
    );


#endif // __ACE_RELATIONS_H__
