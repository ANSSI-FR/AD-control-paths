/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Inherited")
#define PLUGIN_KEYWORD      _T("INH")
#define PLUGIN_DESCRIPTION  _T("Filters inherited ACE (remove them by default)");

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
static BYTE gs_MaskFilter = 0;
static LPTSTR gs_InhflagsOpt = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    // TODO : specify exact mask (and allow to use hex)

    DWORD i = 0;

    API_LOG(Bypass, _T("By default, filters out ACE for whom the 'INHERITED_ACE' flag is present"));
    API_LOG(Bypass, _T("If the <inhflags> plugin option is set, keeps ACE with an AceFlags containing at least the specified flags"));
    API_LOG(Bypass, _T("Valid flags which can be specified (comma separated) are : "));
    for (i = 0; i < ARRAY_COUNT(gc_AceFlagsValues); i++) {
        API_LOG(Bypass, _T("- %s <%#02x>"), gc_AceFlagsValues[i].name, gc_AceFlagsValues[i].value);
    }
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD i = 0;
    LPTSTR ctx = NULL;
    LPTSTR flag = NULL;
    gs_InhflagsOpt = api->Common.GetPluginOption(_T("inhflags"), FALSE);

    if (gs_InhflagsOpt) {
        while (api->Common.StrNextToken(gs_InhflagsOpt, _T(","), &ctx, &flag)) {
            for (i = 0; i < ARRAY_COUNT(gc_AceFlagsValues); i++) {
                if (_tcsicmp(flag, gc_AceFlagsValues[i].name) == 0) {
                    gs_MaskFilter |= gc_AceFlagsValues[i].value;
                    break;
                }
            }
            if (i == ARRAY_COUNT(gc_AceFlagsValues)) {
                API_FATAL(_T("Unknown inheritance flag <%s>"), flag);
            }
        }
        API_LOG(Info, _T("Filtering inherited flags with mask <%#02x>"), gs_MaskFilter);
    }
    else {
        API_LOG(Info, _T("Filtering out inherited ACE"));
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    UNREFERENCED_PARAMETER(api);
	//
	// Do not check inherited status when ObjectType GUID is present and matches one of the object classes
	//
	if (api->Ace.isObjectTypeClassMatching(ace))
		return TRUE;
    if (gs_InhflagsOpt) {
        return ((ace->imported.raw->AceFlags & gs_MaskFilter) == gs_MaskFilter);
    }
    else {
        return ((ace->imported.raw->AceFlags & INHERITED_ACE) == 0);
    }
}
