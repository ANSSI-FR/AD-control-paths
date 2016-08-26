/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("AdmSdHolder")
#define PLUGIN_KEYWORD      _T("ASH")
#define PLUGIN_DESCRIPTION  _T("Filters out ACE from AdminSdHolder security descriptor");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_ADMINSDHOLDER_SD);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("This filter causes the construction of the AdminSdHolder's SD, which is done by running"));
    API_LOG(Bypass, _T("through all ACEs before filtering. Thus, the processing time is nearly doubled."));
}

BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
	PIMPORTED_OBJECT object = NULL;
	object = api->Resolver.ResolverGetAceObject(ace);
	
    if (object && object->imported.adminCount) {
		return !api->Ace.IsInAdminSdHolder(ace);
    }
    return TRUE;
}
