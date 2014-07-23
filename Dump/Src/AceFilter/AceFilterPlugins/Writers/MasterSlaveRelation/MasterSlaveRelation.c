/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("MasterSlaveRelation")
#define PLUGIN_KEYWORD      _T("MSR")
#define PLUGIN_DESCRIPTION  _T("Outputs ACE in the format master->slave->relation");

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
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_SID_RESOLUTION);


/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_MSR_OUTFILE                 _T("msrout.tsv")
#define DEFAULT_MSR_NO_RELATION_KEYWORD     _T("FILTERED_GENERIC_RELATION")
#define MSR_OUTFILE_HEADER                  _T("dnMaster\tdnSlave\tkeyword\n")
#define MSR_OUTFILE_FORMAT                  _T("%s\t%s\t%s\n")

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_outfileName = NULL;
static HANDLE gs_hOutfile = INVALID_HANDLE_VALUE;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void WriteRelation(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ LPTSTR master,
    _In_ LPTSTR slave,
    _In_ LPTSTR relationKeyword
    ) {
    TCHAR outline[MAX_LINE] = { 0 };
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;
    int size = 0;

    size = _stprintf_s(outline, _countof(outline)-1, MSR_OUTFILE_FORMAT, master, slave, relationKeyword);
    if (size == -1) {
        API_LOG(Err, _T("Failed to format outline <%s : %s>"), master, slave);
        return;
    }
    else {
        bResult = WriteFile(gs_hOutfile, outline, size, &dwWritten, NULL);
        if (!bResult) {
            API_LOG(Err, _T("Failed to write outline <%s : %s>"), master, slave);
        }
    }
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the outfile with the <msrout> plugin option"));
    API_LOG(Bypass, _T("The default outfile is <%s>"), DEFAULT_MSR_OUTFILE);
    API_LOG(Bypass, _T("Some filters does not associate 'relations' (and the corresponding keywords) to ACE they filter,"));
    API_LOG(Bypass, _T("in this case, the generic relation keyword is used in the msr outfile : <%s>"), DEFAULT_MSR_NO_RELATION_KEYWORD);
}

BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;

    gs_outfileName = api->Common.GetPluginOption(_T("msrout"), FALSE);
    if (!gs_outfileName) {
        gs_outfileName = DEFAULT_MSR_OUTFILE;
    }
    API_LOG(Info, _T("Outfile is <%s>"), gs_outfileName);


    gs_hOutfile = CreateFile(gs_outfileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (gs_hOutfile == INVALID_HANDLE_VALUE) {
        API_FATAL(_T("Failed to create outfile <%s> : <%u>"), gs_outfileName, GetLastError());
    }
    bResult = WriteFile(gs_hOutfile, MSR_OUTFILE_HEADER, (DWORD)_tcslen(MSR_OUTFILE_HEADER), &dwWritten, NULL);
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
    DWORD i = 0;
    DWORD relCount = 0;
    LPTSTR resolvedTrustee = NULL;

    resolvedTrustee = api->Resolver.ResolverGetAceTrusteeStr(ace);


    for (i = 0; i < ACE_REL_COUNT; i++) {
        if (HAS_RELATION(ace, i)) {
            relCount++;
            WriteRelation(api, resolvedTrustee, ace->imported.objectDn, api->Ace.GetAceRelationStr(i));
        }
    }

    if (relCount == 0) {
        WriteRelation(api, resolvedTrustee, ace->imported.objectDn, DEFAULT_MSR_NO_RELATION_KEYWORD);
    }

    return TRUE;
}