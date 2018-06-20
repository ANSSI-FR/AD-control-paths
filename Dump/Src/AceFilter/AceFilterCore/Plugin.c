/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "Plugin.h"

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
const static PLUGIN_TYPE gcs_PluginTypeImporter = {
    .name = _T("Importer"),
    .directory = PLUGINS_DIR_IMPORTERS,
    .size = sizeof(PLUGIN_IMPORTER)
};

const static PLUGIN_TYPE gcs_PluginTypeFilter = {
    .name = _T("Filter"),
    .directory = PLUGINS_DIR_FILTERS,
    .size = sizeof(PLUGIN_FILTER)
};

const static PLUGIN_TYPE gcs_PluginTypeWriter = {
    .name = _T("Writer"),
    .directory = PLUGINS_DIR_WRITERS,
    .size = sizeof(PLUGIN_WRITER)
};

/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static FARPROC PluginResolve(
    _Inout_ PPLUGIN_HEAD plugin,
    _In_ LPTSTR procName,
    _In_ BOOL fatal
    ) {
	FARPROC proc = NULL;
	LPSTR procNameA = NULL;
	HANDLE processHeap = NULL;

	// TODO
	processHeap = GetProcessHeap();
	UtilsHeapAllocAStrAndConvertWStr((PUTILS_HEAP)&processHeap, (LPWSTR)procName, &procNameA);
    proc = GetProcAddress(plugin->module, procNameA);
    if (!proc && fatal) {
        FATAL(_T("Failed to resolve required symbol <%s> for plugin <%s>"), procName, PLUGIN_GET_NAME(plugin));
    }
    else {
        LOG(Dbg, _T("Symbol <%s> for plugin <%s> : <%p>"), procName, PLUGIN_GET_NAME(plugin), proc);
    }
    return proc;
}

static void PluginLoadRequirements(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    // This table must be consistent with the "PLUGIN_REQUIREMENT" enum (same count, same order)
    static const LPTSTR sc_req[PLUGIN_REQ_COUNT] = {
        PLUGIN_REQ_STR(PLUGIN_REQUIRE_SID_RESOLUTION),
        PLUGIN_REQ_STR(PLUGIN_REQUIRE_DN_RESOLUTION),
        PLUGIN_REQ_STR(PLUGIN_REQUIRE_GUID_RESOLUTION),
		PLUGIN_REQ_STR(PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION),
        PLUGIN_REQ_STR(PLUGIN_REQUIRE_ADMINSDHOLDER_SD),
    };

    DWORD i = 0;
    PBOOL symbol = NULL;

    for (i = 0; i < PLUGIN_REQ_COUNT; i++) {
#pragma warning(disable:4054)
        symbol = (PBOOL)PluginResolve(plugin, sc_req[i], FALSE);
#pragma warning(default:4054)
        if (symbol && *symbol) {
            PLUGIN_SET_REQUIREMENT(plugin, i);
        }
    }
}

static void PluginLoadGenericSingle(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    TCHAR path[MAX_PATH] = { 0 };
    size_t size = 0;

    size = _stprintf_s(path, _countof(path), _T("%s/%s.%s"), plugin->type->directory, PLUGIN_GET_NAME(plugin), PLUGINS_FILE_EXTENSION);
    if (size == -1) {
        FATAL(_T("Cannot construct path for module <%s> (directory is <%s>)"), PLUGIN_GET_NAME(plugin), plugin->type->directory);
    }

    plugin->module = LoadLibrary(path);
    if (!plugin->module) {
        FATAL(_T("Cannot open module for plugin <%s> : <%u> (path is <%s>)"), PLUGIN_GET_NAME(plugin), GetLastError(), path);
    }
    else {
        LOG(Dbg, _T("Loaded module for plugin <%s> : <%#08x>"), PLUGIN_GET_NAME(plugin), plugin->module);
    }

    PLUGIN_RESOLVE_FN(plugin, PLUGIN_PFN_INITIALIZE, Initialize, PLUGIN_GENERIC_INITIALIZE, FALSE);
    PLUGIN_RESOLVE_FN(plugin, PLUGIN_PFN_FINALIZE, Finalize, PLUGIN_GENERIC_FINALIZE, FALSE);
    PLUGIN_RESOLVE_FN(plugin, PLUGIN_PFN_SIMPLE, Help, PLUGIN_GENERIC_HELP, TRUE);
#pragma warning(disable:4054)
    PLUGIN_RESOLVE_VAR(plugin, LPTSTR, description, PLUGIN_GENERIC_DESCRIPTION, FALSE);
    PLUGIN_RESOLVE_VAR(plugin, LPTSTR, keyword, PLUGIN_GENERIC_KEYWORD, TRUE);
#pragma warning(default:4054)

    PluginLoadRequirements(plugin);
}

static DWORD PluginLoadGenericMultiple(
    _Inout_ LPTSTR names,
    _Inout_ PBYTE pluginsArray,
    _In_ DWORD max,
    _In_ PLUGIN_TYPE const * const type
    ) {
    DWORD count = 0;
    DWORD i = 0;
    LPTSTR pluginName = NULL;
    LPTSTR ctx = NULL;
    PPLUGIN_HEAD plugin = NULL;

    while (StrNextToken(names, _T(","), &ctx, &pluginName)) {
        LOG(Info, SUB_LOG(_T("Loading %s <%s>")), type->name, pluginName);

        if (count >= max) {
            FATAL(_T("Cannot load more than <%u> %ss"), max, type->name);
        }

        for (i = 0; i < count; i++) {
            if (STR_EQ(pluginName, PLUGIN_GET_NAME((PPLUGIN_HEAD)(pluginsArray + (i * type->size))))) {
                FATAL(_T("%s <%s> is already loaded"), type->name, pluginName);
            }
        }

        plugin = (PPLUGIN_HEAD)(pluginsArray + (count * type->size));
        PLUGIN_SET_NAME(plugin, pluginName);
        PLUGIN_SET_TYPE(plugin, type);
        PluginLoadGenericSingle(plugin);

        count++;
    }

    return count;
}

static void ListPluginType(
    _In_ PLUGIN_TYPE const *type
    ) {
    HANDLE dir = NULL;
    WIN32_FIND_DATA fileData = { 0 };
    TCHAR path[MAX_PATH] = { 0 };
    size_t size = 0;
    DWORD count = 0;
    LPTSTR ext = NULL;
    PLUGIN_HEAD plugin = { 0 };

    LOG(Bypass, _T("Plugins of type <%s> :"), type->name);

    PLUGIN_SET_TYPE(&plugin, type);

    size = _stprintf_s(path, _countof(path), _T("%s/*.%s"), type->directory, PLUGINS_FILE_EXTENSION);
    if (size == -1) {
        FATAL(_T("Cannot construct path for plugin type <%s> (directory is <%s>)"), type->name, type->directory);
    }

    dir = FindFirstFile(path, &fileData);
    if (dir == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            LOG(Succ, SUB_LOG(_T("None")));
            return;
        }
        else {
            FATAL(_T("Cannot open path <%s> to list <%s> : <%u>"), path, type->name, GetLastError());
        }
    }

    do {
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            ext = _tcsrchr(fileData.cFileName, '.');
            if (ext) {
                *ext = '\0';
                PLUGIN_SET_NAME(&plugin, fileData.cFileName);
                PluginLoadGenericSingle(&plugin);
                LOG(Bypass, SUB_LOG(_T("[%s] %s : %s")), plugin.vars.keyword, plugin.name, PLUGIN_GET_DESCRIPTION(&plugin));
                PluginUnload(&plugin);
                count++;
            }
        }
    } while (FindNextFile(dir, &fileData));

    LOG(Bypass, SUB_LOG(_T("Count : %u\r\n")), count);

    FindClose(dir);
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void ListImporters() {
    ListPluginType(&gcs_PluginTypeImporter);
}

void ListFilters() {
    ListPluginType(&gcs_PluginTypeFilter);
}

void ListWriters() {
    ListPluginType(&gcs_PluginTypeWriter);
}

DWORD PluginLoadImporters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_IMPORTER plugins,
    _In_ DWORD max
    ) {
    DWORD count = 0;
    DWORD i = 0;

    count = PluginLoadGenericMultiple(names, (PBYTE)plugins, max, &gcs_PluginTypeImporter);

    for (i = 0; i < count; i++) {
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_PROCESSACE, GetNextAce, PLUGIN_IMPORTER_GETNEXTACE, FALSE);
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_PROCESSSCHEMA, GetNextSchema, PLUGIN_IMPORTER_GETNEXTSCH, FALSE);
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_PROCESSOBJECT, GetNextObject, PLUGIN_IMPORTER_GETNEXTOBJ, FALSE);
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_RESETREADING, ResetReading, PLUGIN_IMPORTER_RESETREADING, TRUE);
		PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_FREEACE, FreeAce, PLUGIN_IMPORTER_FREEACE, FALSE);
		PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_FREEOBJECT, FreeObject, PLUGIN_IMPORTER_FREEOBJECT, FALSE);
		PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_FREESCHEMA, FreeSchema, PLUGIN_IMPORTER_FREESCHEMA, FALSE);
        PLUGIN_SET_LOADED(&plugins[i]);
    }
    return count;
}

DWORD PluginLoadFilters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_FILTER plugins,
    _In_ DWORD max
    ) {
    DWORD count = 0;
    DWORD i = 0;

    count = PluginLoadGenericMultiple(names, (PBYTE)plugins, max, &gcs_PluginTypeFilter);

    for (i = 0; i < count; i++) {
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_PROCESSACE, FilterAce, PLUGIN_FILTER_FILTERACE, TRUE);
        PLUGIN_SET_LOADED(&plugins[i]);
    }

    return count;
}

DWORD PluginLoadWriters(
    _In_ LPTSTR names,
    _Inout_ PPLUGIN_WRITER plugins,
    _In_ DWORD max
    ) {
    DWORD count = 0;
    DWORD i = 0;

    count = PluginLoadGenericMultiple(names, (PBYTE)plugins, max, &gcs_PluginTypeWriter);

    for (i = 0; i < count; i++) {
        PLUGIN_RESOLVE_FN(&plugins[i], PLUGIN_PFN_PROCESSACE, WriteAce, PLUGIN_WRITER_WRITEACE, TRUE);
        PLUGIN_SET_LOADED(&plugins[i]);
    }

    return count;
}

BOOL PluginInitialize(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    LOG(Info, SUB_LOG(_T("Initializing plugin <%s>")), PLUGIN_GET_NAME(plugin));

    if (plugin->functions.Initialize) {
        BOOL bResult = plugin->functions.Initialize(&gc_PluginApiTable);
        if (!bResult){
            FATAL(SUB_LOG(_T("Cannot initialize plugin <%s>")), PLUGIN_GET_NAME(plugin));
        }
        return bResult;
    }

    LOG(Dbg, SUB_LOG(_T("Plugin <%s> does not have an initializer")), PLUGIN_GET_NAME(plugin));
    return TRUE;
}

BOOL PluginFinalize(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    LOG(Info, SUB_LOG(_T("Finalizing plugin <%s>")), PLUGIN_GET_NAME(plugin));

    if (plugin->functions.Finalize) {
        BOOL bResult = plugin->functions.Finalize(&gc_PluginApiTable);
        if (!bResult){
            LOG(Err, SUB_LOG(_T("Cannot finalize plugin <%s>")), PLUGIN_GET_NAME(plugin));
        }
        return bResult;
    }

    LOG(Dbg, SUB_LOG(_T("Plugin <%s> does not have an finalizer")), PLUGIN_GET_NAME(plugin));
    return TRUE;
}

BOOL PluginUnload(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    LOG(Info, SUB_LOG(_T("Unloading plugin <%s>")), PLUGIN_GET_NAME(plugin));

    BOOL bResult = FreeLibrary(plugin->module);
    if (!bResult) {
        LOG(Warn, SUB_LOG(_T("Cannot close plugin <%s> : <%u>")), PLUGIN_GET_NAME(plugin), GetLastError());
    }
    return bResult;
}

BOOL PluginHelp(
    _Inout_ PPLUGIN_HEAD plugin
    ) {
    LOG(Bypass, _T("------------------------------"));
    LOG(Bypass, _T("Help for %s <%s> :"), plugin->type->name, PLUGIN_GET_NAME(plugin));
    LOG(Bypass, SUB_LOG(_T("Description : %s")), PLUGIN_GET_DESCRIPTION(plugin));
    plugin->functions.Help(&gc_PluginApiTable);
    LOG(Bypass, EMPTY_STR);

    return TRUE;
}
