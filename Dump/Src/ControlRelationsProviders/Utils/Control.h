#ifndef __CONTROL_H__
#define __CONTROL_H__

#define UTILS_REQUIRE_GETOPT_COMPLEX
#define STATIC_GETOPT

/* --- INCLUDES ------------------------------------------------------------- */
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <Sddl.h>
#include <Aclapi.h>
#include <Winldap.h>
#include <io.h>
#include <fcntl.h>

#include "UtilsLib.h"
#include "CacheLib.h"
#include "CsvLib.h"
#include "LogLib.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_OUTFILE_HEADER      {_T(":START_ID"),_T(":END_ID"),_T(":TYPE")}
#define CONTROL_DEFAULT_LOGLVL      _T("WARN")
#define OUTFILE_TOKEN_COUNT					3
#define MAX_RID_SIZE                21


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _CONTROL_OPTIONS {
    PTCHAR ptOutfile;
	PTCHAR ptDenyOutfile;
	PTCHAR ptInfile;
    PTCHAR ptLogLevel;
    PTCHAR ptLogFile;
    BOOL bShowHelp;
} CONTROL_OPTIONS, *PCONTROL_OPTIONS;

typedef void (FN_CONTROL_CALLBACK_RESULT)(
    _In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
    _Inout_ LPTSTR * tokens
    );

typedef enum _OBJ_CSV_TOKENS {
	LdpListDn = 0,
	LdpListObjectClass = 1,
	LdpListObjectSid = 2,
	LdpListAdminCount = 3,
	LdpListMember = 4,
	LdpListGPLink = 5,
	LdpListPrimaryGroupID = 6,
	LdpListSIDHistory = 7,
	LdpListCN = 8,
	LdpListManagedBy,
	LdpListRevealOnDemand,
	LdpListNeverReveal,
	LdpListMail,
	LdpListHomeMDB,
	LdpListMsExchRoleEntries,
	LdpListMsExchUserLink,
	LdpListMsExchRoleLink,
	LdpListUAC,
	LdpListAllowedToDelegateTo,
	LdpListAllowedToActOnBehalf,
	LdpListSPN,
	LdpListDnsHostName,
} OBJ_CSV_TOKENS;

typedef enum _ACE_CSV_TOKENS {
	LdpAceDn = 0,
	LdpAceSd = 1,
} OBJ_CSV_TOKENS;

typedef enum _REL_CSV_TOKENS {
	RelDnMaster = 0,
	RelDnSlave = 1,
	RelKeyword
} REL_CSV_TOKENS;

typedef struct _CACHE_OBJECT_BY_SID {
	LPTSTR sid;
	LPTSTR dn;
} CACHE_OBJECT_BY_SID, *PCACHE_OBJECT_BY_SID;

typedef struct _CACHE_OBJECT_BY_RID {
	LPTSTR rid;
	LPTSTR dn;
} CACHE_OBJECT_BY_RID, *PCACHE_OBJECT_BY_RID;

typedef struct _CACHE_OBJECT_BY_DN {
	LPTSTR dn;
	LPTSTR objectClass;
} CACHE_OBJECT_BY_DN, *PCACHE_OBJECT_BY_DN;

typedef struct _CACHE_MAIL_BY_DN {
	LPTSTR dn;
	LPTSTR mail;
} CACHE_MAIL_BY_DN, *PCACHE_MAIL_BY_DN;
	
typedef struct _CACHE_OBJECT_BY_SPN {
	LPTSTR spn;
	LPTSTR dn;
} CACHE_OBJECT_BY_SPN, *PCACHE_OBJECT_BY_SPN;

typedef struct _CACHE_OBJECT_BY_DNSHOSTNAME {
	LPTSTR dnshostname;
	LPTSTR dn;
} CACHE_OBJECT_BY_DNSHOSTNAME, *PCACHE_OBJECT_BY_DNSHOSTNAME;

typedef void (FN_USAGE_CALLBACK)(
	_In_ PTCHAR progName
	);

/* --- VARIABLES ------------------------------------------------------------ */
DWORD gs_recordNumber;
BOOL bCacheBuilt;
PCACHE ppCache;
PCACHE ppMbxCache;
PCACHE ppSpnCache;
PCACHE ppDnsCache;
FN_USAGE_CALLBACK GenericUsage;


/* --- PROTOTYPES ----------------------------------------------------------- */
void ControlParseOptions(
    _Inout_ PCONTROL_OPTIONS pControlOptions,
    _In_ int argc,
    _In_ PTCHAR argv[]
    );

void ControlUsage(
    );

CSV_HANDLE ControlOpenOutfile(
    _In_ PTCHAR ptOutfile
    );

BOOL ControlWriteOutline(
    _In_ CSV_HANDLE hOutfile,
    _In_ PTCHAR ptMaster,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    );

BOOL ControlWriteOwnerOutline(
    _In_ CSV_HANDLE hOutfile,
    _In_ PSECURITY_DESCRIPTOR pSdOwner,
    _In_ PTCHAR ptSlave,
    _In_ PTCHAR ptKeyword
    );

void CallbackBuildSidCache(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_In_ LPTSTR *tokens
);

void ControlMainForeachCsvResult(
	_In_ int argc,
	_In_ PTCHAR argv[],
	_In_ PTCHAR *outfileHeader,
	_In_ FN_CONTROL_CALLBACK_RESULT pfnCallback,
	_In_ FN_USAGE_CALLBACK pfnUsage
);

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompare(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
);

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompareRid(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
);

RTL_GENERIC_COMPARE_RESULTS _Function_class_(RTL_AVL_COMPARE_ROUTINE)  pfnCompareDn(
	__in struct _RTL_AVL_TABLE  *Table,
	__in PVOID  FirstStruct,
	__in PVOID  SecondStruct
);

#endif // __CONTROL_H__
