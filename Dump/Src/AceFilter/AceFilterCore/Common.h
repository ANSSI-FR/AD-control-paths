/******************************************************************************\
\******************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <Sddl.h>
#include <Strsafe.h>
#include <Aclapi.h>

#ifdef VLD_DBG
    #define VLD_FORCE_ENABLE
    #include <vld.h>
    #pragma comment(lib,"vld.lib")
#endif


/* --- DEFINES -------------------------------------------------------------- */
// Those 2 macros allow the use of the "do { ... } while(false) syntax for multiline
// macros whitout having a "conditional expression is constant" warning
#define MULTI_LINE_MACRO_BEGIN      do {
#define MULTI_LINE_MACRO_END                                        \
    __pragma(warning(push))         \
    __pragma(warning(disable:4127)) \
} while (0)                         \
    __pragma(warning(pop))

// Logging macros
#define LOG_CHR_All     " "
#define LOG_CHR_Dbg     "?"
#define LOG_CHR_Info    "."
#define LOG_CHR_Warn    "!"
#define LOG_CHR_Err     "-"
#define LOG_CHR_Succ    "+"
#define LOG_CHR_Bypass  " "

#define LOG_CHR(lvl)                _T("[") ## _T(LOG_CHR_ ## lvl) ## _T("] ")
#define LOG(lvl, frmt, ...)         LOG_NO_NL(lvl, NONE(frmt) ## _T("\r\n"), __VA_ARGS__);
#define LOG_NO_NL(lvl, frmt, ...)   Log(lvl, LOG_CHR(lvl) ## frmt, __VA_ARGS__);
#define FATAL(frmt, ...)            MULTI_LINE_MACRO_BEGIN              \
    LOG(Err, frmt, __VA_ARGS__);    \
    ExitProcess(EXIT_FAILURE);      \
    MULTI_LINE_MACRO_END

#define FCT_FRMT(frmt)              _T(" --[") ## _T(__FUNCTION__) ## _T("] ") ## frmt
#define SUB_LOG(frmt)               _T(" -- ") ## frmt
#define LOG_FCT(lvl, frmt, ...)     LOG(lvl, FCT_FRMT(frmt), __VA_ARGS__);
#define FATAL_FCT(frmt, ...)        FATAL(FCT_FRMT(frmt), __VA_ARGS__);
#define FATAL_DBG()                 FATAL(_T("[DBG_BREAK] %s : %s : %u"), __FILE__, __FUNCTION__, __LINE__)
#define LOG_FRMT_TIME               _T("[%02u:%02u:%02u] ")

// Bitmap macros
#define DECLARE_BITMAP(name, count) BYTE name[ ROUND_UP(count, BITS_IN_BYTE) / BITS_IN_BYTE ]
#define SET_BIT(bmp, idx)           (bmp)[ idx / BITS_IN_BYTE ] |= (1 << (idx % BITS_IN_BYTE))
#define GET_BIT(bmp, idx)           (((bmp)[ idx / BITS_IN_BYTE ] & (1 << (idx % BITS_IN_BYTE))) >> (idx % BITS_IN_BYTE))

// Sizes
#define MAX_LINE                    (4096)
#define BITS_IN_BYTE                (8)

// Misc
#define BIT(i)                      (1 << (i))
#define ARRAY_COUNT(x)              (sizeof(x) / sizeof((x)[0]))
#define ROUND_UP(N, S)              ((((N) + (S) - 1) / (S)) * (S))
#define PERCENT(count, total)       (total ? ( (((float)(count)) * 100.0) / total ) : 0)
#define TIME_DIFF_SEC(start, stop)  (((stop) - (start)) / 1000.0)
#define STR(x)                      _T(#x)
#define CHR(x)                      _T(#@x)
#define CONCAT(a,b)                 a ## b
#define NONE(x)                     x
#define EMPTY_STR                   _T("")
#define SEPARATOR_STR               _T("------------------------------")
#define DW_CLASS_SCHEMA             (0x0003000d)
#define ADMIN_SD_HOLDER_PARTIAL_DN  _T("CN=AdminSDHolder,CN=System,DC=")
#define BAD_POINTER                 ((PVOID)-1)
#define BAD(p)                      ((p) == BAD_POINTER)
#define NULL_IF_BAD(p)              (BAD(p) ? NULL : (p))
#define STR_EMPTY(s)                (!(s) || (s)[0] == _T('\0'))
#define STR_EQ(a,b)                 (_tcscmp(a,b) == 0)
#define GUID_EQ(pg1, pg2)           (memcmp((PVOID)pg1, (PVOID)pg2, sizeof(GUID)) == 0)
#define GUID_EMPTY(pg)              (MemoryIsNull((PBYTE)(pg), sizeof(GUID)))
#define GUID_STR_SIZE               (8 + 2*4 + 8*2 + 4*1 + 1 + 1)   // hexified : 1*dword + 2*short + 8*char + 4*'-' + '{' + '}'
#define GUID_STR_SIZE_NULL          (GUID_STR_SIZE + 1)             // + null
#define SID_EMPTY(ps, len)          (MemoryIsNull((PBYTE)(ps), len ? len : GetLengthSid(ps)))


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _LOG_LEVEL {
    All = 0,
    Dbg = 1,
    Info = 2,
    Warn = 3,
    Err = 4,
    Succ = 5,
    None = 1337,    // leets dont need logs
    Bypass = 9001,  // IT'S OVER 9000
} LOG_LEVEL;

typedef unsigned __int64 QWORD;

typedef struct _STR_PAIR_LIST {
    struct _STR_PAIR_LIST * next;
    LPTSTR name;
    LPTSTR val;
} STR_PAIR_LIST, *PSTR_PAIR_LIST;

typedef struct _NUMERIC_CONSTANT {
    LPTSTR name;
    DWORD value;
} NUMERIC_CONSTANT, *PNUMERIC_CONSTANT;

/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
void SetLogLevel(
    _In_ LPTSTR lvl
    );

BOOL SetLogFile(
    _In_ LPTSTR logFile
    );

void Log(
    _In_  LOG_LEVEL   lvl,
    _In_  LPTSTR      frmt,
    _In_  ...
    );

BOOL IsInSetOfStrings(
    _In_  LPCTSTR        arg,
    _In_  const LPCTSTR  stringSet[],
    _In_  DWORD          setSize,
    _Out_opt_ DWORD          *index
    );

BOOL MemoryIsNull(
    _In_ PBYTE mem,
    _In_ DWORD len
    );

BOOL StrNextToken(
    _In_ LPTSTR str,
    _In_ LPTSTR del,
    _Inout_ LPTSTR *ctx,
    _Out_ LPTSTR *tok
    );

void Hexify(
    _Inout_  LPTSTR outstr,
    _In_ PBYTE indata,
    _In_ DWORD len
    );

void Unhexify(
    _Inout_ PBYTE outdata,
    _In_ LPTSTR instr
    );

void AddStrPair(
    _Inout_ PSTR_PAIR_LIST *end,
    _In_ LPTSTR name,
    _In_ LPTSTR value
    );

LPTSTR GetStrPair(
    _In_ PSTR_PAIR_LIST head,
    _In_ LPTSTR name
    );

void DestroyStrPairList(
    _In_ PSTR_PAIR_LIST head
    );

HLOCAL LocalAllocCheck(
    _In_  LPTSTR   szCallerName,
    _In_  SIZE_T   uBytes
    );

HLOCAL LocalReAllocCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HLOCAL   hMem,
    _In_  SIZE_T   uBytes
    );

void LocalFreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HLOCAL   hMem
    );

LPVOID HeapAllocCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HANDLE   hHeap,
    _In_  DWORD    dwFlags,
    _In_  SIZE_T   dwBytes
    );

void HeapFreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HANDLE   hHeap,
    _In_  DWORD    dwFlags,
    _In_  LPVOID   lpMem
    );

LPTSTR StrDupCheck(
    _In_  LPTSTR   szCallerName,
    _In_  LPTSTR   szStrToDup
    );

void FreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_ void *memblock
    );

#define LocalAllocCheckX(x)         LocalAllocCheck(_T(__FUNCTION__), x)
#define LocalFreeCheckX(x)          LocalFreeCheck(_T(__FUNCTION__), x)
#define HeapAllocCheckX(x,y,z)      HeapAllocCheck(_T(__FUNCTION__), x, y, z)
#define HeapFreeCheckX(x,y,z)       HeapFreeCheck(_T(__FUNCTION__), x, y, z)
#define StrDupCheckX(x)             StrDupCheck(_T(__FUNCTION__), x)
#define FreeCheckX(x)               FreeCheck(_T(__FUNCTION__), x)

BOOL IsAceInSd(
    _In_ PACE_HEADER pHeaderAceToCheck,
    _In_ PSECURITY_DESCRIPTOR pSd
    );

DWORD * ParseObjectClasses(
    _Inout_  LPTSTR   szObjectClasses,
    _Out_    DWORD    *pdwObjectClassCount
    );

HRESULT ConvertStrGuidToGuid(
    _In_ LPTSTR strGuid,
    _Out_ GUID * guid
    );

BOOL SetPrivilege(
    _Inout_  HANDLE   hToken,
    _In_     LPCTSTR  lpszPrivilege,
    _In_     BOOL     bEnablePrivilege
    );

BOOL EnablePrivForCurrentProcess(
    _In_ LPTSTR szPrivilege
    );


#endif // __COMMON_H__