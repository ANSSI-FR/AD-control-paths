/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("DefaultSd")
#define PLUGIN_KEYWORD      _T("DSD")
#define PLUGIN_DESCRIPTION  _T("Removes ACE from NtDefaultSecurityDescriptor of object classes");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION);


/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Filters out ACE coming from the default SD of one of the classes of the object on which the ACE applies"));
    API_LOG(Bypass, _T("This requires a conversion from SDDL, so we use static Bigrams translation to succeed on outside-domain machines."));
    API_LOG(Bypass, _T("This filter does not use any plugin options"));
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
	return !api->Ace.IsInDefaultSd(ace);
}
