/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("InheritedObjectType")
#define PLUGIN_KEYWORD      _T("IOT")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by InheritedObjectType");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static GUID gs_InhObjectTypeFilter = { 0 };
static BOOL gs_EmptyInhObjType = FALSE;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Keeps ACE with an InheritedObjectType matching the one specified in the <inhobjtype> plugin option"));
    API_LOG(Bypass, _T("If <inhobjtype> is not specified, keeps Object-ACE with an empty InheritedObjectType"));
    API_LOG(Bypass, _T("<inhobjtype> must be a GUID, enclosed or not in curly braces"));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;
    LPTSTR inhobjty = api->Common.GetPluginOption(_T("inhobjtype"), FALSE);
    // TODO : guid resolution from resolved str form could be nice here

    if (!inhobjty) {
        API_LOG(Info, _T("Filtering empty inheritedObjectType"));
        gs_EmptyInhObjType = TRUE;
    }
    else {
		bResult = api->Common.ConvertStrGuidToGuid(inhobjty, &gs_InhObjectTypeFilter);
        gs_EmptyInhObjType = FALSE;
        if (bResult != NOERROR) {
            API_FATAL(_T("Error while converting inherited object type guid <%s> to its binary form : <%#08x)"), inhobjty, bResult);
        }
        else {
            API_LOG(Info, _T("Filtering inheritedObjectType <%s>"), inhobjty);
        }
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    DWORD flags = 0;
    GUID * inheritedObjectType = NULL;

    if (IS_OBJECT_ACE(ace->imported.raw)) {
        flags = api->Ace.GetObjectFlags(ace);
        if (gs_EmptyInhObjType) {
            return (BOOL)((flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) == 0);
        }
        else {
            if (flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
                inheritedObjectType = api->Ace.GetInheritedObjectTypeAce(ace);
                return GUID_EQ(inheritedObjectType, &gs_InhObjectTypeFilter);
            }
        }
    }

    return TRUE;
}
