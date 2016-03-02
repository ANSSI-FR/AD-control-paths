#ifndef __LDAP_DUMP_H__
#define __LDAP_DUMP_H__


/* --- INCLUDES ------------------------------------------------------------- */
#include "Utils.h"
#include "Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_OPT_OUTFILE_ACE         _T("ace.ldpdmp.tsv")
#define DEFAULT_OPT_OUTFILE_OBJECT      _T("obj.ldpdmp.tsv")
#define DEFAULT_OPT_OUTFILE_SCHEMA      _T("sch.ldpdmp.tsv")
#define DEFAULT_OPT_LOG_LEVEL           _T("WARN")

#define LDAP_DUMP_ATTR_COUNT_ACE        (1)
#define LDAP_DUMP_ATTR_COUNT_OBJ        (3)
#define LDAP_DUMP_ATTR_COUNT_SCH        (4)

#define LDAP_DUMP_FILTER_NTSD           _T("(") ## NONE(LDAP_ATTR_NTSD) ## _T("=*)")
#define LDAP_DUMP_FILTER_ALL_OBJECTS    _T("(") ## NONE(LDAP_ATTR_OBJECT_CLASS) ## _T("=*)")


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _LDAP_DUMP_OPTIONS {
    struct {
        BOOL bDumpAce;
        BOOL bDumpObjectList;
        BOOL bDumpSchema;
    } actions;

    struct {
        LPTSTR szOutfileAce;
        LPTSTR szOutfileObjectList;
        LPTSTR szOutfileSchema;
    } outfiles;

    LDAP_OPTIONS ldap;

    struct {
        BOOL bShowHelp;
        LPTSTR szLogFile;
        LPTSTR szLogLevel;
        LPTSTR szRestrictDumpDn;
    } misc;

} LDAP_DUMP_OPTIONS, *PLDAP_DUMP_OPTIONS;

// must be consistent with LDAP_DUMP_ATTR_COUNT_ACE
typedef enum _LDAP_DUMP_ATTR_ACE {
    AttrAce_ntSecurityDescriptor = 0
} LDAP_DUMP_ATTR_ACE;

// must be consistent with LDAP_DUMP_ATTR_COUNT_OBJ
typedef enum _LDAP_DUMP_ATTR_OBJ {
    AttrObj_objectClass = 0,
    AttrObj_objectSid = 1,
    AttrObj_adminCount = 2,
} LDAP_DUMP_ATTR_OBJ;

// must be consistent with LDAP_DUMP_ATTR_COUNT_SCH
typedef enum _LDAP_DUMP_ATTR_SCH {
    AttrSch_schemaIDGUID = 0,
    AttrSch_governsID = 1,
    AttrSch_defaultSecurityDescriptor = 2,
	AttrSch_lDAPDisplayName = 3,
} LDAP_DUMP_ATTR_SCH;

typedef enum _LDAP_DUMP_ATTR_FORMAT {
    AttrStr,
    AttrHex,
} LDAP_DUMP_ATTR_FORMAT;


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */


#endif // __LDAP_DUMP_H__
