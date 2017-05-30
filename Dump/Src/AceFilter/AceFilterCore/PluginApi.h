/******************************************************************************\
\******************************************************************************/

#ifndef __PLUGIN_API_H__
#define __PLUGIN_API_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "ImportedObjects.h"
#include "AceConstants.h"



/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
typedef struct _PLUGIN_API_TABLE {
    struct {
        void(*Log)(
			_In_  const LOG_LEVEL   eLoglvl,
			_In_  const PTCHAR ptFrmt,
			_In_  ...
        );

        LPTSTR(*GetPluginOption)(
            _In_ LPTSTR name,
            _In_ BOOL required
            );

        BOOL (*ConvertStrGuidToGuid)(
			_In_ const PTCHAR ptGuid,
			_In_ GUID * pGuid
            );

        BOOL (*StrNextToken)(
			_In_ const PTCHAR ptStr,
			_In_ const PTCHAR ptDelim,
			_Inout_ PTCHAR *pptCtx,
			_Out_ PTCHAR *pptTok
            );

        BOOL (*IsInSetOfStrings)(
			_In_ const PTCHAR ptNeedle,
			_In_ const PTCHAR ptHaystack[],
			_In_ const DWORD dwSetSize,
			_Out_opt_ DWORD *pdwIndex
            );

        void (*Hexify)(
            _In_ LPTSTR outstr,
            _In_ PBYTE indata,
            _In_ DWORD len
            );

        void (*Unhexify)(
            _In_ PBYTE outdata,
            _In_ LPTSTR instr
            );

        BOOL (*EnablePrivForCurrentProcess)(
			_In_ const PTCHAR ptPriv
            );

    } Common;

	struct {
		BOOL (*CsvOpenRead)(
			_In_ const LPTSTR ptCsvFilename,
			_Out_opt_ PDWORD pdwCsvHeaderCount,
			_Out_opt_ LPTSTR *ppptCsvHeaderValues[],
			_Out_ PCSV_HANDLE pCsvHandle
		);
		BOOL (*CsvGetNextRecord)(
			_In_ const CSV_HANDLE hCsvHandle,
			_In_ LPTSTR **pptCsvRecordValues,
			_Out_opt_ PDWORD pdwCsvRecordNumber
		);
		BOOL (*CsvClose)(
			_Inout_ PCSV_HANDLE phCsvHandle
		);
		BOOL (*CsvResetFile)(
			_In_ const CSV_HANDLE hCsvHandle
		);
		VOID (*CsvHeapFree)(
			_In_ PVOID pMem
			);
		VOID (*CsvRecordArrayHeapFree)(
			_In_ PVOID *ppMemArr,
			_In_ DWORD dwCount
		);
		BOOL (*CsvOpenWrite)(
			_In_ const LPTSTR ptCsvFilename,
			_In_ const DWORD dwCsvHeaderCount,
			_In_ const LPTSTR pptCsvHeaderValues[],
			_Out_ PCSV_HANDLE pCsvHandle
		);
		BOOL (*CsvWriteNextRecord)(
			_In_ const CSV_HANDLE hCsvHandle,
			_In_ const LPTSTR pptCsvRecordValues[],
			_Out_opt_ PDWORD pdwCsvRecordNumber
		);
		DWORD (*CsvGetLastError)(
			_In_ const CSV_HANDLE hCsvHandle
		);
	} InputCsv;

    struct {
		BOOL (*UtilsHeapCreate)(
			_Out_ PUTILS_HEAP *ppHeap,
			_In_ const PTCHAR ptName,
			_In_opt_ const PFN_UTILS_HEAP_FATAL_ERROR_HANDLER pfnFatalErrorHandler
		);

		BOOL (*UtilsHeapDestroy)(
			_Inout_ PUTILS_HEAP *ppHeap
		);

		_Ret_notnull_ PVOID (*UtilsHeapAlloc)(
			_In_ const PTCHAR ptCaller,
			_In_ const PUTILS_HEAP pHeap,
			_In_ const DWORD dwSize
		);

		_Ret_notnull_ PVOID (*UtilsHeapStrDup)(
			_In_ const PTCHAR ptCaller,
			_In_ const PUTILS_HEAP pHeap,
			_In_ const PTCHAR ptStr
		);

		_Ret_notnull_ PVOID (*UtilsHeapMemDup)(
			_In_ const PTCHAR ptCaller,
			_In_ const PUTILS_HEAP pHeap,
			_In_ const PVOID pvStruct,
			_In_ const DWORD dwSize
		);

		void (*UtilsHeapFree)(
			_In_ const PTCHAR ptCaller,
			_In_ const PUTILS_HEAP pHeap,
			_In_ const PVOID pMem
		);

		void(*UtilsHeapFreeArray)(
			_In_ const PTCHAR ptCaller,
			_In_ const PUTILS_HEAP pHeap,
			_In_ const PVOID *ppMemArr,
			_In_ const DWORD dwCount
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

		BOOL(*isObjectTypeClassMatching)(
			_In_ PIMPORTED_ACE ace
			);

		BOOL(*isObjectTypeClass)(
			_In_ PIMPORTED_ACE ace
			);

    } Ace;

    struct {

        LPTSTR (*ResolverGetAceTrusteeStr)(
        _In_ PIMPORTED_ACE ace
        );
       
        PIMPORTED_OBJECT(*ResolverGetAceTrustee)(
        _In_ PIMPORTED_ACE ace
        );

        PIMPORTED_OBJECT (*ResolverGetAceObject)(
            _In_ PIMPORTED_ACE ace
            );

		LPTSTR(*ResolverGetAceObjectMail)(
			_In_ PIMPORTED_ACE ace
			);

        PSECURITY_DESCRIPTOR (*ResolverGetSchemaDefaultSD)(
            _In_ PIMPORTED_SCHEMA sch
            );

        PIMPORTED_OBJECT(*ResolverGetSchemaObject)(
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

		GUID *(*GetAdmPwdGuid)(
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
