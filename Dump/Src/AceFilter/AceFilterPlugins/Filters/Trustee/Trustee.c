/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Trustee")
#define PLUGIN_KEYWORD      _T("TEE")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by Trustee (SID or DN)");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
#define SID_STR_PREFIX _T("S-1-")

/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PSID gs_TrusteeFilter = NULL;
static LPTSTR gs_TrusteeDnFilter = NULL;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the DN or SID of the trustee to be filtered with the <trustee> plugin option (no default)"));
    API_LOG(Bypass, _T("If a SID is specified, it musts start with <%s> to be valid"), SID_STR_PREFIX);
    API_LOG(Bypass, _T("ACE granting permissions to trustees matching the specified values are kept."));
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;
    LPTSTR trustee = api->Common.GetPluginOption(_T("trustee"), TRUE);

    if (_tcsncmp(SID_STR_PREFIX, trustee, _tcslen(SID_STR_PREFIX)) == 0) {
        bResult = ConvertStringSidToSid(trustee, &gs_TrusteeFilter);
        if (!bResult) {
            API_FATAL(_T("Failed to convert SID <%s> to its binary form : <%u>"), trustee, GetLastError());
        }
        API_LOG(Info, _T("Filtering trustee by SID <%s>"), trustee);
    }
    else {
        // Resolution will take place later since resolver has not been activated yet
        gs_TrusteeDnFilter = trustee;
        API_LOG(Info, _T("Filtering trustee by DN <%s>"), trustee);
    }

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    PSID trusteeSidAce = api->Ace.GetTrustee(ace);

    if (gs_TrusteeFilter == NULL && gs_TrusteeDnFilter != NULL) {
        PIMPORTED_OBJECT object = api->Resolver.GetObjectByDn(gs_TrusteeDnFilter);
        if (!object) {
            API_FATAL(_T("Cannot resolve DN <%s>"), gs_TrusteeDnFilter);
        }
        gs_TrusteeFilter = &object->imported.sid;
    }
	if (!gs_TrusteeFilter) {
		API_FATAL(_T("Obj with empty trustee filter <%s>"), gs_TrusteeDnFilter);
	}
    return EqualSid(trusteeSidAce, gs_TrusteeFilter);
}
