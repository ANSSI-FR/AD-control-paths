/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("AceMask")
#define PLUGIN_KEYWORD      _T("MSK")
#define PLUGIN_DESCRIPTION  _T("Filters ACE by access mask");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FILTERACE;

/* --- DEFINES -------------------------------------------------------------- */
/* --- TYPES ---------------------------------------------------------------- */
typedef struct _MASK_RIGHTS {
    LPTSTR str;
    int flag;
} MASK_RIGHTS, *PMASK_RIGHTS;

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
// TODO : use AceConstants
static const MASK_RIGHTS gs_RightsFlags[] = {
    { _T("DELETE"), ADS_RIGHT_DELETE },
    { _T("READ_CONTROL"), ADS_RIGHT_READ_CONTROL },
    { _T("WRITE_DAC"), ADS_RIGHT_WRITE_DAC },
    { _T("WRITE_OWNER"), ADS_RIGHT_WRITE_OWNER },
    { _T("SYNCHRONIZE"), ADS_RIGHT_SYNCHRONIZE },
    { _T("ACCESS_SYSTEM_SECURITY"), ADS_RIGHT_ACCESS_SYSTEM_SECURITY },
    { _T("GENERIC_READ"), ADS_RIGHT_GENERIC_READ },
    { _T("GENERIC_WRITE"), ADS_RIGHT_GENERIC_WRITE },
    { _T("GENERIC_EXECUTE"), ADS_RIGHT_GENERIC_EXECUTE },
    { _T("GENERIC_ALL"), ADS_RIGHT_GENERIC_ALL },
    { _T("DS_CREATE_CHILD"), ADS_RIGHT_DS_CREATE_CHILD },
    { _T("DS_DELETE_CHILD"), ADS_RIGHT_DS_DELETE_CHILD },
    { _T("ACTRL_DS_LIST"), ADS_RIGHT_ACTRL_DS_LIST },
    { _T("RIGHT_DS_SELF"), ADS_RIGHT_DS_SELF },
    { _T("DS_READ_PROP"), ADS_RIGHT_DS_READ_PROP },
    { _T("DS_WRITE_PROP"), ADS_RIGHT_DS_WRITE_PROP },
    { _T("DS_DELETE_TREE"), ADS_RIGHT_DS_DELETE_TREE },
    { _T("DS_LIST_OBJECT"), ADS_RIGHT_DS_LIST_OBJECT },
    { _T("DS_CONTROL_ACCESS"), ADS_RIGHT_DS_CONTROL_ACCESS }
};

static ACCESS_MASK gs_AccessMaskFilter = 0;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    // TODO : specify exact mask (and allow to use hex)
    // TODO : take into account FS flags

    DWORD i = 0;

    API_LOG(Bypass, _T("Keeps ACE with an AccessMask containing at least the flags specified in the <mask> plugin option") );
    API_LOG(Bypass, _T("Valid flags which can be specified (comma separated) are : "));
    for (i = 0; i < ARRAY_COUNT(gs_RightsFlags); i++) {
        API_LOG(Bypass, _T("- %s <%#02x>"), gs_RightsFlags[i].str, gs_RightsFlags[i].flag);
    }
}


BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD i = 0;
    LPTSTR ctx = NULL;
    LPTSTR flag = NULL;
    LPTSTR mask = api->Common.GetPluginOption(_T("mask"), FALSE);

    while (api->Common.StrNextToken(mask, _T(","), &ctx, &flag)) {
        for (i = 0; i < ARRAY_COUNT(gs_RightsFlags); i++) {
            if (_tcsicmp(flag, gs_RightsFlags[i].str) == 0) {
                gs_AccessMaskFilter |= gs_RightsFlags[i].flag;
                break;
            }
        }
        if (i == ARRAY_COUNT(gs_RightsFlags)) {
            API_FATAL(_T("Unknown access mask flag <%s>"), flag);
        }
    }
    API_LOG(Info, _T("Filtering access mask <%#02x>"), gs_AccessMaskFilter);

    return TRUE;
}


BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    ACCESS_MASK aceMask = api->Ace.GetAccessMask(ace);
    return ((aceMask & gs_AccessMaskFilter) == gs_AccessMaskFilter);
}
