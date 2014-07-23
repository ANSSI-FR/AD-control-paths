/******************************************************************************\
\******************************************************************************/


#ifndef __INPUT_FILE_H__
#define __INPUT_FILE_H__


/* --- INCLUDES ------------------------------------------------------------- */
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "Common.h"


/* --- DEFINES -------------------------------------------------------------- */
#define MAX_SIZE_LINE                       (131072)
#define INPUT_FILE_PERCENT_PROGRESS_DELTA   (5)


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _INPUT_FILE {
    LPTSTR         szName;
    LPTSTR         szPath;
    HANDLE         hFile;
    HANDLE         hMapping;
    PVOID          pvMapping;
    LARGE_INTEGER  fileSize;
} INPUT_FILE, *PINPUT_FILE;

typedef void FN_LINE_CALLBACK(
    _In_     DWORD    dwLineNumber,
    _In_     LPTSTR   pszTokens[],
    _In_     DWORD    dwTokenCount,
    _Inout_  PVOID    pParam
    );
typedef FN_LINE_CALLBACK *PFN_LINE_CALLBACK;


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
DWORD ParseLine(
    _In_  PCHAR    szLine,
    _In_  DWORD    dwMaxLineSize,
    _In_  DWORD    dwTokenCount,
    _Inout_ LPTSTR   pszTokens[]
    );

BOOL ReadLine(
    _In_                          PBYTE    *pInput,
    _In_                          LONGLONG llSizeInput,
    _Inout_                         LONGLONG *llPositionInput,
    _Out_writes_(dwMaxLineSize)   PTCHAR   pOutputLine,
    _In_                          DWORD    dwMaxLineSize
    );

DWORD ReadParseTsvLine(
    _In_                          PBYTE    *pInput,
    _In_                          LONGLONG llSizeInput,
    _Inout_                         LONGLONG *llPositionInput,
    _In_  DWORD    dwTokenCount,
    _Inout_ LPTSTR   pszTokens[],
    _In_ LPTSTR skipHeaderFirstToken
    );

void ForeachLine(
    _In_     PINPUT_FILE       pInputFile,
    _In_     DWORD             dwTokenCount,
    _In_     FN_LINE_CALLBACK  pfnCallback,
    _Inout_  PVOID             pParam
    );

BOOL InitInputFile(
    _In_     LPTSTR      szPath,
    _In_     LPTSTR      szName,
    _Inout_  PINPUT_FILE pInputFile
    );

void ResetInputFile(
    _In_ PINPUT_FILE inputFile,
    _Inout_ PVOID *mapping,
    _Inout_ PLONGLONG position
    );

void CloseInputFile(
    _Inout_  PINPUT_FILE pInputFile
    );

#endif // __INPUT_FILE_H__
