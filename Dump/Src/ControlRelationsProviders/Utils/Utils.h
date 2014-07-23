#ifndef __UTILS_H__
#define __UTILS_H__

#pragma warning(disable:4706) // warning : assignment within conditional expression


/* --- INCLUDES ------------------------------------------------------------- */
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <Sddl.h>
#include <Aclapi.h>
#include "Getopt.h"

#ifdef VLD_DBG
    #define VLD_FORCE_ENABLE
    #include <vld.h>
    #pragma comment(lib,"vld.lib")
#endif


/* --- DEFINES -------------------------------------------------------------- */
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

#define MAX_LINE                    4096
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

typedef void (FN_USAGE_CALLBACK)(
    _In_ PTCHAR progName
);


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
FN_USAGE_CALLBACK GenericUsage;

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
    _Out_ DWORD          *index
    );

BOOL StrNextToken(
    _In_ LPTSTR str,
    _In_ LPTSTR del,
    _Inout_ LPTSTR *ctx,
    _Out_ LPTSTR *tok
    );

BOOL SetPrivilege(
    _Inout_  HANDLE   hToken,
    _In_     LPCTSTR  lpszPrivilege,
    _In_     BOOL     bEnablePrivilege
    );

BOOL EnablePrivForCurrentProcess(
    _In_ LPTSTR szPrivilege
    );

BOOL IsDomainSid(
    _In_ PSID pSid
    );

HANDLE FileOpenWithBackupPriv(
    _In_ PTCHAR ptPath,
    _In_ BOOL bUseBackupPriv
    );


#endif // __UTILS_H__
