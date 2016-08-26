/******************************************************************************\
\******************************************************************************/

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "ImportedObjects.h"
#include "PluginApi.h"


/* --- DEFINES -------------------------------------------------------------- */
#define PLUGINS_FILE_EXTENSION              _T("dll")
#define PLUGINS_DIR                         "Plugins"
#define PLUGINS_DIR_IMPORTERS               _T(PLUGINS_DIR ## "/" ## "Importers")
#define PLUGINS_DIR_FILTERS                 _T(PLUGINS_DIR ## "/" ## "Filters")
#define PLUGINS_DIR_WRITERS                 _T(PLUGINS_DIR ## "/" ## "Writers")

#define PLUGIN_GENERIC(plugin)              ((PPLUGIN_HEAD)plugin)

#define PLUGIN_SET_NAME(plugin, n)          PLUGIN_GENERIC(plugin)->name = n;
#define PLUGIN_SET_TYPE(plugin, t)          PLUGIN_GENERIC(plugin)->type = t;
#define PLUGIN_RESOLVE_FN(p, t, f, m, b)    (p)->functions.f = (t)PluginResolve(PLUGIN_GENERIC(p), STR(m), b);
#define PLUGIN_RESOLVE_VAR(p, t, v, m, b)   (p)->vars.v = (t)PluginResolve(PLUGIN_GENERIC(p), STR(m), b);
#define PLUGIN_SET_LOADED(plugin)           PLUGIN_GENERIC(plugin)->loaded = TRUE;
#define PLUGIN_IS_LOADED(plugin)            (PLUGIN_GENERIC(plugin)->loaded)
#define PLUGIN_GET_NAME(plugin)             (PLUGIN_GENERIC(plugin)->name)
#define PLUGIN_GET_DESCRIPTION(plugin)      (PLUGIN_GENERIC(plugin)->vars.description ? PLUGIN_GENERIC(plugin)->vars.description : PLUGIN_DEFAULT_DESCRIPTION)
#define PLUGIN_DEFAULT_DESCRIPTION          _T("<no description>")

#define PLUGIN_IMPORTER_GETNEXTACE          AceFilterPlugin_Importer_fn_GetNextAce
#define PLUGIN_IMPORTER_GETNEXTOBJ          AceFilterPlugin_Importer_fn_GetNextObject
#define PLUGIN_IMPORTER_GETNEXTSCH          AceFilterPlugin_Importer_fn_GetNextSchema
#define PLUGIN_IMPORTER_FREEACE             AceFilterPlugin_Importer_fn_FreeAce
#define PLUGIN_IMPORTER_FREEOBJECT          AceFilterPlugin_Importer_fn_FreeObject
#define PLUGIN_IMPORTER_FREESCHEMA          AceFilterPlugin_Importer_fn_FreeSchema
#define PLUGIN_IMPORTER_RESETREADING        AceFilterPlugin_Importer_fn_ResetReading
#define PLUGIN_FILTER_FILTERACE             AceFilterPlugin_Filter_fn_FilterAce
#define PLUGIN_WRITER_WRITEACE              AceFilterPlugin_Writer_fn_WriteAce
#define PLUGIN_GENERIC_INITIALIZE           AceFilterPlugin_fn_Initialize
#define PLUGIN_GENERIC_FINALIZE             AceFilterPlugin_fn_Finalize
#define PLUGIN_GENERIC_HELP                 AceFilterPlugin_fn_Help
#define PLUGIN_GENERIC_NAME                 AceFilterPlugin_gc_Name
#define PLUGIN_GENERIC_DESCRIPTION          AceFilterPlugin_gc_Description
#define PLUGIN_GENERIC_KEYWORD              AceFilterPlugin_gc_Keyword

#define PLUGIN_REQUIRE_SID_RESOLUTION       AceFilterPlugin_gc_ReqSidResolution
#define PLUGIN_REQUIRE_DN_RESOLUTION        AceFilterPlugin_gc_ReqDnResolution
#define PLUGIN_REQUIRE_GUID_RESOLUTION      AceFilterPlugin_gc_ReqGuidResolution
#define PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION	AceFilterPlugin_gc_ReqDisplayNameResolution
#define PLUGIN_REQUIRE_ADMINSDHOLDER_SD     AceFilterPlugin_gc_ReqAdmSdHolderSd

#define PLUGIN_REQ_COUNT                    (5) // must be consistent with the number of values in the PLUGIN_REQUIREMENT enum
#define PLUGIN_SET_REQUIREMENT(p, req)      BITMAP_SET_BIT(PLUGIN_GENERIC(p)->requirements, req)
#define PLUGIN_REQUIRES(p, req)             BITMAP_GET_BIT(PLUGIN_GENERIC(p)->requirements, req)
#define PLUGIN_REQ_STR(x)                   STR(x) // STR cannot be used directly, since it is defined before the PLUGIN_REQUIRE_* macros


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _PLUGIN_REQUIREMENT {
    OPT_REQ_SID_RESOLUTION,
    OPT_REQ_DN_RESOLUTION,
    OPT_REQ_GUID_RESOLUTION,
	OPT_REQ_DISPLAYNAME_RESOLUTION,
    OPT_REQ_ADMINSDHOLDER_SD,
} PLUGIN_REQUIREMENT;
// TODO : __COUNTER__ would be nice here, but it's already used in an included header. unless there's a way to reset it ?
// static_assert(__COUNTER__ == PLUGIN_REQ_COUNT, "inconsistent value for OPT_REQU_COUNT");

typedef enum _IMPORTER_DATATYPE {
    ImporterAce,
    ImporterObject,
    ImporterSchema,
} IMPORTER_DATATYPE;

typedef BOOL PLUGIN_FN_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    );
typedef BOOL PLUGIN_FN_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    );
typedef BOOL PLUGIN_FN_PROCESSACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    );
typedef BOOL PLUGIN_FN_PROCESSSCHEMA(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_SCHEMA sch
    );
typedef BOOL PLUGIN_FN_PROCESSOBJECT(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_OBJECT obj
    );
typedef void PLUGIN_FN_RESETREADING(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ IMPORTER_DATATYPE type
    );
typedef void PLUGIN_FN_SIMPLE(
    _In_ PLUGIN_API_TABLE const * const table
    );
typedef void PLUGIN_FN_FREEACE(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_ACE ace
    );
typedef void PLUGIN_FN_FREEOBJECT(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_OBJECT obj
    );
typedef void PLUGIN_FN_FREESCHEMA(
	_In_ PLUGIN_API_TABLE const * const api,
	_Inout_ PIMPORTED_SCHEMA sch
    );


typedef PLUGIN_FN_INITIALIZE *PLUGIN_PFN_INITIALIZE;
typedef PLUGIN_FN_FINALIZE *PLUGIN_PFN_FINALIZE;
typedef PLUGIN_FN_PROCESSACE *PLUGIN_PFN_PROCESSACE;
typedef PLUGIN_FN_PROCESSSCHEMA *PLUGIN_PFN_PROCESSSCHEMA;
typedef PLUGIN_FN_PROCESSOBJECT *PLUGIN_PFN_PROCESSOBJECT;
typedef PLUGIN_FN_RESETREADING *PLUGIN_PFN_RESETREADING;
typedef PLUGIN_FN_SIMPLE *PLUGIN_PFN_SIMPLE;
typedef PLUGIN_FN_FREEACE *PLUGIN_PFN_FREEACE;
typedef PLUGIN_FN_FREEOBJECT *PLUGIN_PFN_FREEOBJECT;
typedef PLUGIN_FN_FREESCHEMA *PLUGIN_PFN_FREESCHEMA;


typedef struct _PLUGIN_TYPE {
    LPTSTR name;
    LPTSTR directory;
    DWORD size;
} PLUGIN_TYPE, *PPLUGIN_TYPE;

typedef struct _PLUGIN_HEAD {
    LPTSTR name;
    BOOL loaded;
    HMODULE module;
    PLUGIN_TYPE const *type;
    DECLARE_BITMAP(requirements, PLUGIN_REQ_COUNT);

    struct {
        PLUGIN_PFN_INITIALIZE Initialize;
        PLUGIN_PFN_FINALIZE Finalize;
        PLUGIN_PFN_SIMPLE Help;
    } functions;

    struct {
        LPTSTR description;
        LPTSTR keyword;
    } vars;

} PLUGIN_HEAD, *PPLUGIN_HEAD;

typedef BOOL(PLUGIN_FN_GENERIC)(
    _Inout_ PPLUGIN_HEAD plugin
    );

typedef struct _IMPORTER_FUNCTIONS {
    PLUGIN_PFN_PROCESSACE GetNextAce;
    PLUGIN_PFN_PROCESSSCHEMA GetNextSchema;
    PLUGIN_PFN_PROCESSOBJECT GetNextObject;
    PLUGIN_PFN_RESETREADING ResetReading;
	PLUGIN_PFN_FREEACE FreeAce;
	PLUGIN_PFN_FREEOBJECT FreeObject;
	PLUGIN_PFN_FREESCHEMA FreeSchema;
} IMPORTER_FUNCTIONS, *PIMPORTER_FUNCTIONS;

typedef struct _FILTER_FUNCTIONS {
    PLUGIN_PFN_PROCESSACE FilterAce;
} FILTER_FUNCTIONS, *PFILTER_FUNCTIONS;

typedef struct _WRITER_FUNCTIONS {
    PLUGIN_PFN_PROCESSACE WriteAce;
} WRITER_FUNCTIONS, *PWRITER_FUNCTIONS;

typedef struct _PLUGIN_IMPORTER {
    PLUGIN_HEAD head;
    IMPORTER_FUNCTIONS functions;
} PLUGIN_IMPORTER, *PPLUGIN_IMPORTER;

typedef struct _PLUGIN_FILTER {
    PLUGIN_HEAD head;
    BOOL inverted;
    FILTER_FUNCTIONS functions;
} PLUGIN_FILTER, *PPLUGIN_FILTER;

typedef struct _PLUGIN_WRITER {
    PLUGIN_HEAD head;
    WRITER_FUNCTIONS functions;
} PLUGIN_WRITER, *PPLUGIN_WRITER;

/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */
void ListImporters();
void ListFilters();
void ListWriters();

DWORD PluginLoadImporters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_IMPORTER plugins,
    _In_ DWORD max
    );

DWORD PluginLoadFilters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_FILTER plugins,
    _In_ DWORD max
    );

DWORD PluginLoadWriters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_WRITER plugins,
    _In_ DWORD max
    );

PLUGIN_FN_GENERIC PluginInitialize;
PLUGIN_FN_GENERIC PluginFinalize;
PLUGIN_FN_GENERIC PluginHelp;
PLUGIN_FN_GENERIC PluginUnload;


#endif // __PLUGIN_H__