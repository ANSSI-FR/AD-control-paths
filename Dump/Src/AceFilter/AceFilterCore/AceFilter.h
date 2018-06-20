/******************************************************************************\
\******************************************************************************/

#ifndef __ACE_FILTER_H__
#define __ACE_FILTER_H__

#define UTILS_REQUIRE_GETOPT_COMPLEX
#define STATIC_GETOPT

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"
#include "Cache.h"
#include "Plugin.h"
#include "ImportedObjects.h"
//#include "getopt.h"



/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_OPT_DEBUGLEVEL      _T("WARN")
#define DEFAULT_OPT_PROGRESSION     (1000000)

#define PLUGIN_MAX_IMPORTERS        (3)     // at most, one for each type of imported objects : ACE, OBJ, SCH
#define PLUGIN_MAX_FILTERS          (20)    // arbitrary
#define PLUGIN_MAX_WRITERS          (20)    // arbitrary


/* --- TYPES ---------------------------------------------------------------- */
typedef struct _ACE_FILTER_OPTIONS {
    struct {
        LPTSTR importers;
        LPTSTR filters;
        LPTSTR writers;
        LPTSTR inverted;
    } names;

    struct {
        PLUGIN_IMPORTER importers[PLUGIN_MAX_IMPORTERS];
        PLUGIN_FILTER filters[PLUGIN_MAX_FILTERS];
        PLUGIN_WRITER writers[PLUGIN_MAX_WRITERS];

        DWORD numberOfImporters;
        DWORD numberOfFilters;
        DWORD numberOfWriters;
    } plugins;

    struct {
        BOOL showHelp;
        LPTSTR logfile;
        LPTSTR loglvl;
        DWORD progression;
    } misc;

} ACE_FILTER_OPTIONS, *PACE_FILTER_OPTIONS;

typedef struct _ACE_FILTER_PLUGIN_OPTIONS {
    PSTR_PAIR_LIST head;
    PSTR_PAIR_LIST end;
} ACE_FILTER_PLUGIN_OPTIONS, *PACE_FILTER_PLUGIN_OPTIONS;


/* --- VARIABLES ------------------------------------------------------------ */
/* --- PROTOTYPES ----------------------------------------------------------- */


#endif // __ACE_FILTER_H__