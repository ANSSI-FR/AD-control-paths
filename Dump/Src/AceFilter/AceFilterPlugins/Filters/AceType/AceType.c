/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("AceType")
#define PLUGIN_KEYWORD      _T("TYP")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by type");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
typedef struct _ACE_TYPES {
    LPTSTR str;
    BYTE type;
} ACE_TYPES, *PACE_TYPES;

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static DWORD gs_AceTypesCount = 0;
static DWORD gs_AceTypesFilter[ARRAY_COUNT(gc_AceTypeValues)] = { 0 };
static LPTSTR gs_AceTypesFiltersStr[ARRAY_COUNT(gc_AceTypeValues)] = { NULL };


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD i = 0;

    API_LOG(Bypass, _T("Keeps ACE of one of the types specified with the <type> plugin option"));
    API_LOG(Bypass, _T("Valid types which can be specified (comma separated) are : "));
    for (i = 0; i < ARRAY_COUNT(gc_AceTypeValues); i++) {
        API_LOG(Bypass, _T("- %s <%#02x>"), gc_AceTypeValues[i].name, gc_AceTypeValues[i].value);
    }
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD i = 0;
    LPTSTR ctx = NULL;
    LPTSTR type = NULL;
    LPTSTR acetypes = api->Common.GetPluginOption(_T("type"), TRUE);

    while (api->Common.StrNextToken(acetypes, _T(","), &ctx, &type)) {
        if (api->Common.IsInSetOfStrings(type, gs_AceTypesFiltersStr, gs_AceTypesCount, NULL)) {
            API_LOG(Warn, _T("Ace type <%s> is specified multiple times"), type);
        }
        else {
            for (i = 0; i < ARRAY_COUNT(gc_AceTypeValues); i++) {
                if (_tcsicmp(type, gc_AceTypeValues[i].name) == 0) {
                    API_LOG(Info, _T("Filtering ace of type <%s>"), gc_AceTypeValues[i].name);
                    gs_AceTypesFiltersStr[gs_AceTypesCount] = gc_AceTypeValues[i].name;
                    gs_AceTypesFilter[gs_AceTypesCount] = gc_AceTypeValues[i].value;
                    gs_AceTypesCount++;
                    break;
                }
            }
            if (i == ARRAY_COUNT(gc_AceTypeValues)) {
                API_FATAL(_T("Unknown ace type <%s>"), type);
            }
        }
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    UNREFERENCED_PARAMETER(api);

    DWORD i = 0;

    for (i = 0; i < gs_AceTypesCount; i++) {
        if (ace->imported.raw->AceType == gs_AceTypesFilter[i]) {
            return TRUE;
        }
    }

    return FALSE;
}
