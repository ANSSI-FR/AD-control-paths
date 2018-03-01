/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Inherited")
#define PLUGIN_KEYWORD      _T("INH")
#define PLUGIN_DESCRIPTION  _T("Filters inherit-only ACE");

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

    API_LOG(Bypass, _T("Filters out ACE when the 'INHERIT_ONLY_ACE' flag is present."));
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
        API_LOG(Info, _T("Filtering out inherit-only ACE"));
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
	UNREFERENCED_PARAMETER(api);

	// InheritedObjectType: when an ACE is inherited, if IOT guid does not match the final Class (Object Category), the inherit_only flag is set.
	// It means the ACE does not apply to the current object. It is still propagated as specified.

	//Version 1:
	// All inherited ACEs are filtered. We do this because we grant control on the original object independently of its class
	// e.g. reset-pwd on an OU even though it's inherit-only/impacts users only.
	/*
	if (gs_InhflagsOpt) {
		return ((ace->imported.raw->AceFlags & gs_MaskFilter) == gs_MaskFilter);
	}
	else {
		return ((ace->imported.raw->AceFlags & INHERITED_ACE) == 0);
	}
	*/

	//Version 2:
	// Inherit-only means ACE does not apply to the current object. Else, return TRUE and let other filters deal with matching object classes.
	// Also, Inherited ACE are not applied to objects with admincount set. 
	// This can happen when getting admincount afterwards: inheritance is "disabled" but this does not remove ACEs from the SD.
	if (gs_InhflagsOpt) {
		return ((ace->imported.raw->AceFlags & gs_MaskFilter) == 0);
	}
	if (ace->imported.raw->AceFlags & INHERIT_ONLY_ACE) {
		return FALSE;
	}
	if (ace->imported.raw->AceFlags & INHERITED_ACE) {
		PIMPORTED_OBJECT obj = api->Resolver.ResolverGetAceObject(ace);
		if(obj && obj->imported.adminCount == 1)
			return FALSE;
	}
	return TRUE;
}
