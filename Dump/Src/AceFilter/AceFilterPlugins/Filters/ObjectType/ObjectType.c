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

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_GUID_RESOLUTION);

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
	API_LOG(Bypass, _T("Filters ACE with an ObjectType GUID of a non-matching class"));
 //   API_LOG(Bypass, _T("Keeps ACE with an ObjectType matching the one specified in the <objtype> plugin option"));
 //   API_LOG(Bypass, _T("If <objtype> is not specified, keeps Object-ACE with an empty ObjectType"));
 //   API_LOG(Bypass, _T("<objtype> must be a GUID, enclosed or not in curly braces"));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;
    LPTSTR objtype = api->Common.GetPluginOption(_T("objtype"), FALSE);

    if (!objtype) {
        API_LOG(Info, _T("Filtering when objectType is a non-matching Class"));
        gs_EmptyObjType = TRUE;
    }
    else {
        bResult = api->Common.ConvertStrGuidToGuid(objtype, &gs_ObjectTypeFilter);
        gs_EmptyObjType = FALSE;
        if (bResult != NOERROR) {
            API_FATAL(_T("Error while converting object type guid <%s> to its binary form : <%#08x)"), objtype, bResult);
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
	// A very important misexplanation of MSDN: ObjectType of a class does not only applies to create/delete child allowed class
	// But to the current object for all other rights if OT class is matching.
	// The ACE does not apply these rights if it's not matching. It's NOT like a deny ACE though !
	if (api->Ace.isObjectTypeClass(ace))
		if (!api->Ace.isObjectTypeClassMatching(ace))
			return FALSE;
	
	return TRUE;
}
