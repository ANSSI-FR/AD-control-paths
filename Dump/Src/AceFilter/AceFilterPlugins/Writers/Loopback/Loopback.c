/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("Loopback")
#define PLUGIN_KEYWORD      _T("LBW")
#define PLUGIN_DESCRIPTION  _T("Outputs ACE in a format that can be read by the 'Loopback' importer");

// Plugin informations
PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

// Plugin functions
PLUGIN_DECLARE_INITIALIZE;
PLUGIN_DECLARE_FINALIZE;
PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_WRITEACE;

// Plugin requirements
/* none */


/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_LBW_OUTFILE                 _T("loopback.tsv")
#define LBW_OUTFILE_HEADER                  _T("imported.objectDn\timported.hexAce\timported.source\n")
#define LBW_OUTFILE_FORMAT                  _T("%s\t%s\t%d\n")

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_outfileName = NULL;
static HANDLE gs_hOutfile = INVALID_HANDLE_VALUE;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the outfile with the <loopout> plugin option (default <%s>)"), DEFAULT_LBW_OUTFILE);
    API_LOG(Bypass, _T("When using multiple writers, this one must be placed last in the command-line"));
}

BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;

    gs_outfileName = api->Common.GetPluginOption(_T("loopout"), FALSE);
    if (!gs_outfileName) {
        gs_outfileName = DEFAULT_LBW_OUTFILE;
    }
    API_LOG(Info, _T("Outfile is <%s>"), gs_outfileName);


    gs_hOutfile = CreateFile(gs_outfileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (gs_hOutfile == INVALID_HANDLE_VALUE) {
        API_FATAL(_T("Failed to create outfile <%s> : <%u>"), gs_outfileName, GetLastError());
    }
    bResult = WriteFile(gs_hOutfile, LBW_OUTFILE_HEADER, (DWORD)_tcslen(LBW_OUTFILE_HEADER), &dwWritten, NULL);
    if (!bResult) {
        API_FATAL(_T("Failed to write header to outfile <%s> : <%u>"), gs_outfileName, GetLastError());
    }

    return TRUE;
}

BOOL PLUGIN_GENERIC_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    if (!CloseHandle(gs_hOutfile)) {
        API_LOG(Err, _T("Failed to close outfile handle : <%u>"), GetLastError());
        return FALSE;
    }
    return TRUE;
}

BOOL PLUGIN_WRITER_WRITEACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    TCHAR outline[MAX_LINE] = { 0 };
    TCHAR hexace[MAX_LINE] = { 0 };
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;
    int size = 0;

    api->Common.Hexify(hexace, (PBYTE)ace->imported.raw, ace->imported.raw->AceSize);
    size = _stprintf_s(outline, _countof(outline), LBW_OUTFILE_FORMAT, ace->imported.objectDn, hexace, ace->imported.source);
    if (size == -1) {
        API_LOG(Err, _T("Failed to format outline for ace <%u>"), ace->computed.number);
        return FALSE;
    }
    else {
        bResult = WriteFile(gs_hOutfile, outline, size, &dwWritten, NULL);
        if (!bResult) {
            API_LOG(Err, _T("Failed to write outline for ace <%u>"), ace->computed.number);
            return FALSE;
        }
    }

    return TRUE;
}