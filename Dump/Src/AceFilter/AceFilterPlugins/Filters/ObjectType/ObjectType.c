/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("ObjectType")
#define PLUGIN_KEYWORD      _T("OTY")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by ObjectType");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static GUID gs_ObjectTypeFilter = { 0 };
static BOOL gs_EmptyObjType = FALSE;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Keeps ACE with an ObjectType matching the one specified in the <objtype> plugin option"));
    API_LOG(Bypass, _T("If <objtype> is not specified, keeps Object-ACE with an empty ObjectType"));
    API_LOG(Bypass, _T("<objtype> must be a GUID, enclosed or not in curly braces"));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    HRESULT hr = NOERROR;
    LPTSTR objtype = api->Common.GetPluginOption(_T("objtype"), FALSE);
    // TODO : guid resolution from resolved str form could be nice here

    if (!objtype) {
        API_LOG(Info, _T("Filtering empty objectType"));
        gs_EmptyObjType = TRUE;
    }
    else {
        hr = api->Common.ConvertStrGuidToGuid(objtype, &gs_ObjectTypeFilter);
        gs_EmptyObjType = FALSE;
        if (hr != NOERROR) {
            API_FATAL(_T("Error while converting object type guid <%s> to its binary form : <%#08x)"), objtype, hr);
        }
        else {
            API_LOG(Info, _T("Filtering objectType <%s>"), objtype);
        }
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    DWORD flags = 0;
    GUID * objectType = NULL;

    if (IS_OBJECT_ACE(ace->imported.raw)) {
        flags = api->Ace.GetObjectFlags(ace);
        if (gs_EmptyObjType) {
            return ((flags & ACE_OBJECT_TYPE_PRESENT) == 0);
        }
        else {
            if (flags & ACE_OBJECT_TYPE_PRESENT) {
                objectType = api->Ace.GetObjectTypeAce(ace);
                return GUID_EQ(objectType, &gs_ObjectTypeFilter);
            }
        }
    }

    return FALSE;
}
