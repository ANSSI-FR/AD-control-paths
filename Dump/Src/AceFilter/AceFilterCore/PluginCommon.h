/******************************************************************************\
    This file _MUST_ be included by each plugin.
    And must _ONLY_ be included by plugins.
\******************************************************************************/

#ifndef __PLUGIN_COMMON_H__
#define __PLUGIN_COMMON_H__

/* --- INCLUDES ------------------------------------------------------------- */
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

#include "ImportedObjects.h"
#include "AceConstants.h"
#include "Plugin.h"
#include "PluginApi.h"


/* --- DEFINES -------------------------------------------------------------- */
#undef LOG_NO_NL
// This new LOG macro implicitely uses the plugin-api-table (must be named "table"), and the plugin keyword (must be named "PLUGIN_KEYWORD")
#define LOG_NO_NL(lvl, frmt, ...)   api->Common.Log(lvl, LOG_CHR(lvl) ## SUB_LOG(LOG_KEYW() ## frmt), __VA_ARGS__);
#define LOG_KEYW()                  _T("[") ## NONE(PLUGIN_KEYWORD) ## _T("] ")
#define API_LOG                     LOG
#define API_FATAL                   FATAL

#define ApiHeapAllocX(x,y)                     api->Alloc.UtilsHeapAlloc(_T(__FUNCTION__), x, y)
#define ApiHeapFreeX(x,y)                      api->Alloc.UtilsHeapFree(_T(__FUNCTION__), x, y)
#define ApiStrDupX(x,y)                        api->Alloc.UtilsHeapStrDup(_T(__FUNCTION__), x, y)
#define ApiHeapFreeX(x,y)                      api->Alloc.UtilsHeapFree(_T(__FUNCTION__), x, y)
#define ApiHeapFreeArrayX(x,y,z)               api->Alloc.UtilsHeapFreeArray(_T(__FUNCTION__),x,y,z)
#define ApiHeapCreateX(x,y,z)                  api->Alloc.UtilsHeapCreate(x,y,z)
#define ApiHeapDestroyX(x)                     api->Alloc.UtilsHeapDestroy(x)


#define PLUGIN_KEYWORD_SIZE                 (4) // 3 chars + null
#define PLUGIN_NO_SIZE                          // none
#define PLUGIN_EXPORT_VAR(type, name, val)  __declspec(dllexport) const type name = val;
#define PLUGIN_DECLARE_FCT(fctype, fctname) __declspec(dllexport) fctype fctname;
#define PLUGIN_DECLARE_STRING(var, val, s)  PLUGIN_EXPORT_VAR(TCHAR, var[s], val)
#define PLUGIN_DECLARE_REQUIREMENT(req)     PLUGIN_EXPORT_VAR(BOOL, req, TRUE)

#define PLUGIN_DECLARE_KEYWORD              PLUGIN_DECLARE_STRING(PLUGIN_GENERIC_KEYWORD, PLUGIN_KEYWORD, PLUGIN_KEYWORD_SIZE)
#define PLUGIN_DECLARE_NAME                 PLUGIN_DECLARE_STRING(PLUGIN_GENERIC_NAME, PLUGIN_NAME, PLUGIN_NO_SIZE)
#define PLUGIN_DECLARE_DESCRIPTION          PLUGIN_DECLARE_STRING(PLUGIN_GENERIC_DESCRIPTION, PLUGIN_DESCRIPTION, PLUGIN_NO_SIZE)

#define PLUGIN_DECLARE_INITIALIZE           PLUGIN_DECLARE_FCT(PLUGIN_FN_INITIALIZE, PLUGIN_GENERIC_INITIALIZE)
#define PLUGIN_DECLARE_FINALIZE             PLUGIN_DECLARE_FCT(PLUGIN_FN_FINALIZE, PLUGIN_GENERIC_FINALIZE)
#define PLUGIN_DECLARE_HELP                 PLUGIN_DECLARE_FCT(PLUGIN_FN_SIMPLE, PLUGIN_GENERIC_HELP)
#define PLUGIN_DECLARE_GETNEXTACE           PLUGIN_DECLARE_FCT(PLUGIN_FN_PROCESSACE, PLUGIN_IMPORTER_GETNEXTACE)
#define PLUGIN_DECLARE_GETNEXTOBJ           PLUGIN_DECLARE_FCT(PLUGIN_FN_PROCESSOBJECT, PLUGIN_IMPORTER_GETNEXTOBJ)
#define PLUGIN_DECLARE_GETNEXTSCH           PLUGIN_DECLARE_FCT(PLUGIN_FN_PROCESSSCHEMA, PLUGIN_IMPORTER_GETNEXTSCH)
#define PLUGIN_DECLARE_RESETREADING         PLUGIN_DECLARE_FCT(PLUGIN_FN_RESETREADING, PLUGIN_IMPORTER_RESETREADING)
#define PLUGIN_DECLARE_FREEACE              PLUGIN_DECLARE_FCT(PLUGIN_FN_FREEACE, PLUGIN_IMPORTER_FREEACE)
#define PLUGIN_DECLARE_FREEOBJECT           PLUGIN_DECLARE_FCT(PLUGIN_FN_FREEOBJECT, PLUGIN_IMPORTER_FREEOBJECT)
#define PLUGIN_DECLARE_FREESCHEMA           PLUGIN_DECLARE_FCT(PLUGIN_FN_FREESCHEMA, PLUGIN_IMPORTER_FREESCHEMA)
#define PLUGIN_DECLARE_FILTERACE            PLUGIN_DECLARE_FCT(PLUGIN_FN_PROCESSACE, PLUGIN_FILTER_FILTERACE)
#define PLUGIN_DECLARE_WRITEACE             PLUGIN_DECLARE_FCT(PLUGIN_FN_PROCESSACE, PLUGIN_WRITER_WRITEACE)


/* --- TYPES ---------------------------------------------------------------- */
/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */



#endif // __PLUGIN_COMMON_H__