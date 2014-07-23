#ifndef __CONTROL_H__
#define __CONTROL_H__


/* --- INCLUDES ------------------------------------------------------------- */
#include "Utils.h"
#include "Ldap.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_OUTFILE_HEADER      _T("dnMaster\tdnSlave\tkeyword\r\n")
#define CONTROL_OUTFILE_FORMAT      _T("%s\t%s\t%s\r\n")
#define CONTROL_DEFAULT_LOGLVL      _T("WARN")


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _CONTROL_OPTIONS {
    PTCHAR ptOutfile;
    PTCHAR ptLogLevel;
    PTCHAR ptLogFile;
    BOOL bShowHelp;
} CONTROL_OPTIONS, *PCONTROL_OPTIONS;

typedef void (FN_CONTROL_CALLBACK_LDAP_RESULT)(
    _In_ HANDLE hOutfile,
    _Inout_ PLDAP_RETRIEVED_DATA pLdapRetreivedData
    );


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
void ControlParseOptions(
    _Inout_ PCONTROL_OPTIONS pControlOptions,
    _In_ int argc,
    _In_ PTCHAR argv[]
    );

void ControlUsage(
    );

HANDLE ControlOpenOutfile(
    _In_ PTCHAR ptOutfile
    );

BOOL ControlWriteOutline(
    _In_ HANDLE hOutfile,
    _In_ PTCHAR ptMaster,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    );

BOOL ControlWriteOwnerOutline(
    _In_ HANDLE hOutfile,
    _In_ PSECURITY_DESCRIPTOR pSdOwner,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    );

void ControlMainForeachLdapResult(
    _In_ int argc,
    _In_ PTCHAR argv[],
    _In_ PTCHAR ptDefaultOutfile,
    _In_ PTCHAR ptLdapFilter,
    _In_ PTCHAR ptAttribute,
    _In_opt_ PLDAPControl *pLdapServerControls,
    _In_ FN_CONTROL_CALLBACK_LDAP_RESULT pfnCallback,
    _In_ FN_USAGE_CALLBACK pfnUsage
    );


#endif // __CONTROL_H__
