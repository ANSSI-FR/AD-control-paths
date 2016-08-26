/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("ControlFs")
#define PLUGIN_KEYWORD      _T("CFS")
#define PLUGIN_DESCRIPTION  _T("Filters control ACE for files");

PLUGIN_DECLARE_NAME;
PLUGIN_DECLARE_KEYWORD;
PLUGIN_DECLARE_DESCRIPTION;

PLUGIN_DECLARE_HELP;
PLUGIN_DECLARE_FILTERACE;

PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_GUID_RESOLUTION);

/* --- DEFINES -------------------------------------------------------------- */
#define FILE_WRITEDATA_ADDFILE               FILE_WRITE_DATA
#define FILE_APPENDDATA_ADDSUBDIR            FILE_APPEND_DATA

/* --- TYPES ---------------------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Keeps ACE allowing control relations on files. Currently checked control relations are :"));
    API_LOG(Bypass, _T("- Generic rights : GENERIC_ALL, GENERIC_WRITE"));
    API_LOG(Bypass, _T("- Standard rights : WRITE_DAC, WRITE_OWNER"));
    API_LOG(Bypass, _T("- Specific rights : FILE_WRITE_DATA/FILE_ADD_FILE, FILE_APPEND_DATA/FILE_ADD_SUBDIRECTORY"));
    API_LOG(Bypass, _T("This filter set 'relations' on the ACE, and must be combined with the MSR writer for these relations to be outputed."));
}

BOOL PLUGIN_FILTER_FILTERACE(
    _In_ PLUGIN_API_TABLE const * const api,
    _Inout_ PIMPORTED_ACE ace
    ) {
    /*
    - Generic rights :
    --- GENERIC_WRITE
    --- GENERIC_ALL
    - Standards rights :
    --- WRITE_DAC
    --- WRITE_OWNER
    - Files Specific rights (file/dir) :
    --- FILE_WRITE_DATA / FILE_ADD_FILE
    --- FILE_APPEND_DATA / FILE_ADD_SUBDIRECTORY
    */

    DWORD dwAccessMask = 0;
    DWORD i = 0;

    //
    // Get properties
    //
    dwAccessMask = api->Ace.GetAccessMask(ace);

    //
    // Control case : Generic right GENERIC_WRITE
    //
    if (dwAccessMask & ADS_RIGHT_GENERIC_WRITE)
        SET_RELATION(ace, GEN_RIGHT_WRITE);

    //
    // Control case : Generic right GENERIC_ALL
    //
    if (dwAccessMask & ADS_RIGHT_GENERIC_ALL)
        SET_RELATION(ace, GEN_RIGHT_ALL);

    //
    // Control case : Standard right WRITE_DAC
    //
    if (dwAccessMask & ADS_RIGHT_WRITE_DAC)
        SET_RELATION(ace, STAND_RIGHT_WRITE_DAC);

    //
    // Control case : Standard right WRITE_OWNER
    //
    if (dwAccessMask & ADS_RIGHT_WRITE_OWNER)
        SET_RELATION(ace, STAND_RIGHT_WRITE_OWNER);

    //
    // Control case : Specific right FILE_WRITE_DATA / FILE_ADD_FILE
    //
    if (dwAccessMask & FILE_WRITEDATA_ADDFILE)
        SET_RELATION(ace, FS_RIGHT_WRITEDATA_ADDFILE);

    //
    // Control case : Specific right FILE_APPEND_DATA / FILE_ADD_SUBDIRECTORY
    //
    if (dwAccessMask & FILE_APPENDDATA_ADDSUBDIR)
        SET_RELATION(ace, FS_RIGHT_APPENDDATA_ADDSUBDIR);

	//
	// Only "*_ALLOWED_*" ace types can allow control
	// But DENY ace on control parameters cannot be processed on a per-ace model in the control paths approach
	//
	if (!IS_ALLOWED_ACE(ace->imported.raw)) {
		for (i = 0; i < ACE_REL_COUNT; i++) {
			if (HAS_RELATION(ace, i)) {
				API_LOG(Dbg, _T("<%s> control is limited by a DENY %s ACE on object <%s>"), api->Resolver.ResolverGetAceTrusteeStr(ace), api->Ace.GetAceRelationStr(i), api->Resolver.ResolverGetAceObject(ace)->imported.dn);
			}
		}
	}

    for (i = 0; i < ACE_REL_COUNT; i++) {
        if (HAS_RELATION(ace, i)) {
            return TRUE;
        }
    }

    return FALSE;
}