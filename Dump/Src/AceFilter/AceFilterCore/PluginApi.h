/******************************************************************************\
\******************************************************************************/

#ifndef __PLUGIN_API_H__
#define __PLUGIN_API_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "InputFile.h"
#include "ImportedObjects.h"
#include "AceConstants.h"

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
typedef struct _PLUGIN_API_TABLE {

    struct {
        void(*Log)(
        _In_  LOG_LEVEL   lvl,
        _In_  LPTSTR      frmt,
        _In_  ...
        );

        LPTSTR(*GetPluginOption)(
            _In_ LPTSTR name,
            _In_ BOOL required
            );

        HRESULT(*ConvertStrGuidToGuid)(
            _In_ LPTSTR strGuid,
            _Out_ GUID * guid
            );

        BOOL (*StrNextToken)(
            _In_ LPTSTR str,
            _In_ LPTSTR del,
            _Inout_ LPTSTR *ctx,
            _Out_ LPTSTR *tok
            );

        BOOL (*IsInSetOfStrings)(
            _In_  LPCTSTR        arg,
            _In_  const LPCTSTR  stringSet[],
            _In_  DWORD          setSize,
            _Out_opt_ DWORD          *index
            );

        void (*Hexify)(
            _Out_ LPTSTR outstr,
            _In_ PBYTE indata,
            _In_ DWORD len
            );

        void (*Unhexify)(
            _Out_ PBYTE outdata,
            _In_ LPTSTR instr
            );

        BOOL (*EnablePrivForCurrentProcess)(
            _In_ LPTSTR szPrivilege
            );

    } Common;


    struct {
        DWORD(*ParseLine)(
        _In_  PCHAR    szLine,
        _In_  DWORD    dwMaxLineSize,
        _In_  DWORD    dwTokenCount,
        _Inout_ LPTSTR   pszTokens[]
        );

        BOOL(*ReadLine)(
            _In_                          PBYTE    *pInput,
            _In_                          LONGLONG llSizeInput,
            _Out_                         LONGLONG *llPositionInput,
            _Out_writes_(dwMaxLineSize)   PTCHAR   pOutputLine,
            _In_                          DWORD    dwMaxLineSize
            );

        DWORD(*ReadParseTsvLine)(
            _In_                          PBYTE    *pInput,
            _In_                          LONGLONG llSizeInput,
            _Out_                         LONGLONG *llPositionInput,
            _In_  DWORD    dwTokenCount,
            _Out_ LPTSTR   pszTokens[],
            _In_ LPTSTR skipHeaderFirstToken
            );

        void(*ForeachLine)(
            _In_     PINPUT_FILE       pInputFile,
            _In_     DWORD             dwTokenCount,
            _In_     FN_LINE_CALLBACK  pfnCallback,
            _Inout_  PVOID             pParam
            );

        BOOL(*InitInputFile)(
            _In_     LPTSTR      szPath,
            _In_     LPTSTR      szName,
            _Inout_  PINPUT_FILE pInputFile
            );

        void (*ResetInputFile)(
            _In_ PINPUT_FILE inputFile,
            _Inout_ PVOID *mapping,
            _Inout_ PLONGLONG position
            );

        void(*CloseInputFile)(
            _Inout_  PINPUT_FILE pInputFile
            );

    } InputFile;


    struct {

        HLOCAL(*LocalAllocCheck)(
        _In_  LPTSTR   szCallerName,
        _In_  SIZE_T   uBytes
        );

        void(*LocalFreeCheck)(
            _In_  LPTSTR   szCallerName,
            _In_  HLOCAL   hMem
            );

        LPVOID(*HeapAllocCheck)(
            _In_  LPTSTR   szCallerName,
            _In_  HANDLE   hHeap,
            _In_  DWORD    dwFlags,
            _In_  SIZE_T   dwBytes
            );

        void(*HeapFreeCheck)(
            _In_  LPTSTR   szCallerName,
            _In_  HANDLE   hHeap,
            _In_  DWORD    dwFlags,
            _In_  LPVOID   lpMem
            );

        LPTSTR(*StrDupCheck)(
            _In_  LPTSTR   szCallerName,
            _In_  LPTSTR   szStrToDup
            );

        void(*FreeCheck)(
            _In_  LPTSTR   szCallerName,
            _In_ void *memblock
            );

    } Alloc;

    struct {
        ACCESS_MASK(*GetAccessMask)(
        _In_ PIMPORTED_ACE impace
        );

        PSID(*GetTrustee)(
            _In_ PIMPORTED_ACE impace
            );

        DWORD(*GetObjectFlags)(
            _In_ PIMPORTED_ACE impace
            );

        GUID *(*GetObjectTypeAce)(
            _In_ PIMPORTED_ACE impace
            );

        GUID *(*GetInheritedObjectTypeAce)(
            _In_ PIMPORTED_ACE impace
            );

        LPTSTR(*GetAceRelationStr)(
            _In_ ACE_RELATION rel
            );

        BOOL (*IsAceInSd)(
            _In_ PACE_HEADER pHeaderAceToCheck,
            _In_ PSECURITY_DESCRIPTOR pSd
            );

        BOOL(*IsInAdminSdHolder)(
            _In_ PIMPORTED_ACE ace
            );

        BOOL(*IsInDefaultSd)(
            _In_ PIMPORTED_ACE ace
            );

    } Ace;

    struct {

        DWORD * (*ParseObjectClasses)(
        _Inout_  LPTSTR   szObjectClasses,
        _Out_    DWORD    *pdwObjectClassCount
        );

    } Object;

    struct {

        LPTSTR (*ResolverGetAceTrusteeStr)(
        _In_ PIMPORTED_ACE ace
        );

        /*
        LPTSTR (*ResolverGetObjectPrimaryOwnerStr)(
            _In_ PIMPORTED_OBJECT obj
            );
        */

        /*
        LPTSTR (*ResolverGetObjectPrimaryGroupStr)(
            _In_ PIMPORTED_OBJECT obj
            );
        */
        
        PIMPORTED_OBJECT(*ResolverGetAceTrustee)(
        _In_ PIMPORTED_ACE ace
        );

        PIMPORTED_OBJECT (*ResolverGetAceObject)(
            _In_ PIMPORTED_ACE ace
            );

        PIMPORTED_SCHEMA(*ResolverGetObjectObjectClass)(
            _In_ PIMPORTED_OBJECT obj,
            _In_ DWORD idx
            );

        PSECURITY_DESCRIPTOR (*ResolverGetSchemaDefaultSD)(
            _In_ PIMPORTED_SCHEMA sch
            );

        PSECURITY_DESCRIPTOR(*ResolverGetSchemaObject)(
            _In_ PIMPORTED_SCHEMA sch
            );

        PIMPORTED_OBJECT (*GetObjectByDn)(
            _In_ LPTSTR dn
            );

        PIMPORTED_OBJECT (*GetObjectBySid)(
            _In_ PSID sid
            );

        PIMPORTED_SCHEMA (*GetSchemaByGuid)(
            _In_ GUID * guid
            );

        LPTSTR (*GetDomainDn)(
            );

    } Resolver;

} PLUGIN_API_TABLE, *PPLUGIN_API_TABLE;

/* --- VARIABLES ------------------------------------------------------------ */
extern PLUGIN_API_TABLE const gc_PluginApiTable;

/* --- PROTOTYPES ----------------------------------------------------------- */

// declaration of GetPLuginOption, defined in AceFilter.c, wich cannot be included here
extern LPTSTR GetPluginOption(
    _In_ LPTSTR name,
    _In_ BOOL required
    );


#endif // __PLUGIN_API_H__
