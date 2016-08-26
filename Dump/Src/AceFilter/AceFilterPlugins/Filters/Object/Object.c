/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Object")
#define PLUGIN_KEYWORD      _T("OBJ")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by Object on wich the ACE applies (SID or DN)");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_SID_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
#define SID_STR_PREFIX _T("S-1-")

/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_ObjectSidFilterStr = NULL;
static PSID gs_ObjectSidFilter = NULL;
static LPTSTR gs_ObjectDnFilter = NULL;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the DN or SID of the object to be filtered with the <object> plugin option (no default)"));
    API_LOG(Bypass, _T("If a SID is specified, it musts start with <%s> to be valid"), SID_STR_PREFIX);
    API_LOG(Bypass, _T("ACE applying to objects matching the specified values are kept."));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;
    LPTSTR object = api->Common.GetPluginOption(_T("object"), TRUE);

    if (_tcsncmp(SID_STR_PREFIX, object, _tcslen(SID_STR_PREFIX)) == 0) {
        bResult = ConvertStringSidToSid(object, &gs_ObjectSidFilter);
        if (!bResult) {
            API_FATAL(_T("Failed to convert SID <%s> to its binary form : <%u>"), object, GetLastError());
        }
        API_LOG(Info, _T("Filtering trustee by SID <%s>"), object);
        gs_ObjectSidFilterStr = object;
    }
    else {
        gs_ObjectDnFilter = object;
        API_LOG(Info, _T("Filtering trustee by DN <%s>"), object);
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    if (gs_ObjectDnFilter == NULL && gs_ObjectSidFilter != NULL) {
        PIMPORTED_OBJECT object = api->Resolver.GetObjectBySid(gs_ObjectSidFilter);
        if (!object) {
            API_FATAL(_T("Cannot resolve SID <%s>"), gs_ObjectSidFilterStr);
        }
        gs_ObjectDnFilter = object->imported.dn;
    }
	if (!gs_ObjectDnFilter) {
		API_FATAL(_T("Object does not have dn filter <%s>"), gs_ObjectSidFilterStr);
	}
    return STR_EQ(ace->imported.objectDn, gs_ObjectDnFilter);
}
