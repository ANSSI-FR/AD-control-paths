/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Loopback")
#define PLUGIN_KEYWORD      _T("LBI")
#define PLUGIN_DESCRIPTION  _T("Imports ACE from files written by the 'Looback' writer");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FINALIZE;
PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_GETNEXTACE;
PLUGIN_DECLARE_RESETREADING;


/* --- DEFINES -------------------------------------------------------------- */
#define LOOPBACK_ACE_TOKEN_COUNT        (3)
#define LOOPBACK_HEADER_FIRST_TOKEN     _T("imported.objectDn")


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _LOOPBACK_TSV_TOKENS {
    LoopImportedObjectDn = 0,
    LoopImportedHexAce = 1,
    LoopImportedSource = 2,
} LOOPBACK_TSV_TOKENS;


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_LoopFileName = NULL;
static INPUT_FILE gs_LoopFile = { 0 };
static LONGLONG gs_LoopPosition = 0;
static PVOID gs_LoopMapping = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the infile with the <loopin> plugin option (no default)"));
    API_LOG(Bypass, _T("This plugin can only import ACE. You must combine it with others importers that import OBJ and SCH."));
    API_LOG(Bypass, _T("When using multiple importers, this one must be placed first in the command-line"));
}

BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    BOOL bResult = FALSE;

    gs_LoopFileName = api->Common.GetPluginOption(_T("loopin"), TRUE);

    bResult = api->InputFile.InitInputFile(gs_LoopFileName, _T("LOOPACE"), &gs_LoopFile);
    if (!bResult) {
        API_FATAL(_T("Failed to open ace input file <%s>"), gs_LoopFileName);
    }
    else {
        API_LOG(Dbg, _T("Successfully opened file <%s>"), gs_LoopFileName);
    }

    api->InputFile.ResetInputFile(&gs_LoopFile, &gs_LoopMapping, &gs_LoopPosition);

    return TRUE;
}


BOOL PLUGIN_GENERIC_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    api->InputFile.CloseInputFile(&gs_LoopFile);
    return TRUE;
}


void PLUGIN_IMPORTER_RESETREADING(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ IMPORTER_DATATYPE type
    ) {
    UNREFERENCED_PARAMETER(api);

    switch (type) {
    case ImporterAce:
        api->InputFile.ResetInputFile(&gs_LoopFile, &gs_LoopMapping, &gs_LoopPosition);
        break;
    default:
        API_FATAL(_T("Unsupported read-reset operation <%u>"), type);
    }
}

BOOL PLUGIN_IMPORTER_GETNEXTACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    BOOL bResult = FALSE;
    LPTSTR tokens[LOOPBACK_ACE_TOKEN_COUNT] = { 0 };

    bResult = api->InputFile.ReadParseTsvLine((PBYTE *)&gs_LoopMapping, gs_LoopFile.fileSize.QuadPart, &gs_LoopPosition, LOOPBACK_ACE_TOKEN_COUNT, tokens, LOOPBACK_HEADER_FIRST_TOKEN);
    if (!bResult) {
        return FALSE;
    }
    else {
        
        ace->imported.raw = ApiLocalAllocCheckX(_tcslen(tokens[LoopImportedHexAce]) / 2);
        api->Common.Unhexify((PBYTE)ace->imported.raw, tokens[LoopImportedHexAce]);
        ace->imported.source = _tstoi(tokens[LoopImportedSource]);
        ace->imported.objectDn = ApiStrDupCheckX(tokens[LoopImportedObjectDn]);

        if (!IsValidSid(api->Ace.GetTrustee(ace))) {
            API_FATAL(_T("ace does not have a valid trustee sid <%s>"), ace->imported.objectDn);
        }

        return TRUE;
    }
}
