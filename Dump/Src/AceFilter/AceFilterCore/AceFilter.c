/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "AceFilter.h"

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static ACE_FILTER_PLUGIN_OPTIONS gs_PluginOptions = { 0 };

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void Usage(
    _In_  LPCTSTR  progName,
    _In_opt_  LPCTSTR  msg
    ) {
    if (msg) {
        LOG(Err, _T("%s"), msg);
    }

    LOG(Succ, _T("Showing help :"));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> General information :"));
    LOG(Bypass, _T("  This tool's purpose is to generate lists of filtered ACE. For this, it uses different kinds of 'plugins' :"));
    LOG(Bypass, _T("   - importers : used to import ACE and active directory data (object list and schema)"));
    LOG(Bypass, _T("   - filters : used to filter ACE according to various criteria"));
    LOG(Bypass, _T("   - writers : used to outputs lists of filtered ace in the specified format"));
    LOG(Bypass, _T("  You will use these plugins according to the tool used to dump AD data,"));
    LOG(Bypass, _T("  and the tool for which you want to generate filtered lists of ACE."));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> Syntax :"));
    LOG(Bypass, _T("  %s [<--tool-options>=<value>]+ -- [<plugin-options>=<value>]+"), progName);
    LOG(Bypass, _T("  This tool uses 2 types of options : "));
    LOG(Bypass, _T("   - tool-options : described below, starting with a double-dash."));
    LOG(Bypass, _T("   - plugin-options : described in the help of each plugin, in the format <name>=<value>."));
    LOG(Bypass, _T("  Plugins-options must be specified after all tool-options"));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> Help options :"));
    LOG(Bypass, _T("  --help/usage : Shows this help, and the help of each loaded plugins"));
    LOG(Bypass, _T("  --list-plugins : List all plugins (Importers, Filters and Writers)"));
    LOG(Bypass, _T("  --list-(importers|filters|writers) : List plugins of the corresponding category"));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> Plugins options :"));
    LOG(Bypass, _T("  All these options take a comma-separated list of plugins as value"));
    LOG(Bypass, _T("  --importers : list of plugins in charge of loading AD data (e.g. ACE, Object list, and Schema)"));
    LOG(Bypass, _T("    Order matters : the first importer allowing to import one of the 3 data types (ACE/SCH/OBJ)"));
    LOG(Bypass, _T("    will be used for this data. Thus, you can at most specify 3 importers (one for each type)"));
    LOG(Bypass, _T("  --writers : list of plugins used to ouput ACE that pass all filters. Each writer will output the ACE in its specified format."));
    LOG(Bypass, _T("    Order of writers doesn't matter, except for the 'Loopback' writer, which should be placed last."));
    LOG(Bypass, _T("  --filters : list of filters to apply to the ACE list. All filters must be passed for an ACE to be kept."));
    LOG(Bypass, _T("    Place the most restrictive filters first, to save computing time by removing the majority of ACE in the early processing"));
    LOG(Bypass, _T("  --invert-filters : list of filters to invert (previously kept ACE are now filtered for those plugins)"));
    LOG(Bypass, _T("    The concerned plugins must be loaded, so this option must always be specified after the --filters option"));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> Misc options :"));
    LOG(Bypass, _T("  --progression : print a message each time this number of ACE have been processed (default is %u)"), DEFAULT_OPT_PROGRESSION);
    LOG(Bypass, _T("  --loglvl/dbglvl : set the log level. Possibles values are <ALL,DBG,INFO,WARN,ERR,SUCC,NONE> (default is %s)"), DEFAULT_OPT_DEBUGLEVEL);
    LOG(Bypass, _T("  --logfile : save all outputs to a file (none by default)"));
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, SEPARATOR_LINE);
    LOG(Bypass, _T("> Typical examples :"));
    LOG(Bypass, _T("  - Setup recommended logging options :"));
    LOG(Bypass, _T("      %s --loglvl=INFO --logfile=out.log ..."), progName);
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, _T("  - List all plugins with their description :"));
    LOG(Bypass, _T("      %s --list-plugins"), progName);
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, _T("  - Load the chosen plugins, and show help for them :"));
    LOG(Bypass, _T("      %s --importers=Importer1,Importer2 --writers=Writer1 --filters=Filter1,...,FilterN --help"), progName);
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, _T("  - Convert ACE to another format without filtering (no filters specified) :"));
    LOG(Bypass, _T("      %s --importers=Importer --writers=Writer -- pluginopt1=value1 pluginopt2=value2"), progName);
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, _T("  - Filter ACE using filters."));
    LOG(Bypass, _T("    Example 1 : Keep ACCESS_ALLOWED ACE only, non inherited, granting the WRITE_DAC and WRITE_OWNER rights."));
    LOG(Bypass, _T("      %s --imp=Importer --wri=Writer --filters=AceType,Inherited,AceMask -- \\"), progName);
    LOG(Bypass, _T("                    type=ACCESS_ALLOWED mask=WRITE_DAC,WRITE_OWNER"));
    LOG(Bypass, _T("    Example 2 : Remove ACE from AdminSdholder, from default SD, and inherited"));
    LOG(Bypass, _T("      %s --imp=Importer --wri=Writer --filters=Inherited,AdmSdHolder,DefaultSd"), progName);
    LOG(Bypass, EMPTY_STR);
    LOG(Bypass, _T("  - Inverting filters :"));
    LOG(Bypass, _T("    Example 1 : Remove all ACE of type ACCESS_ALLOWED"));
    LOG(Bypass, _T("      %s --imp=Importer --wri=Writer --filters=AceType --invert-filters=AceType -- type=ACCESS_ALLOWED"), progName);
    LOG(Bypass, EMPTY_STR);
}

static void ParseOptions(
    _In_ PACE_FILTER_OPTIONS opt,
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {

    const static struct option long_options[] = {
        { _T("list-plugins"), no_argument, 0, _T('L') },
        { _T("list-importers"), no_argument, 0, _T('I') },
        { _T("list-filters"), no_argument, 0, _T('F') },
        { _T("list-writers"), no_argument, 0, _T('W') },

        { _T("importers"), required_argument, 0, _T('i') },
        { _T("filters"), required_argument, 0, _T('f') },
        { _T("writers"), required_argument, 0, _T('w') },
        { _T("invert-filters"), required_argument, 0, _T('r') },

        { _T("progression"), required_argument, 0, _T('p') },
        { _T("logfile"), required_argument, 0, _T('l') },
        { _T("dbglvl"), required_argument, 0, _T('d') },
        { _T("loglvl"), required_argument, 0, _T('d') },
        { _T("help"), no_argument, 0, _T('h') },
        { _T("usage"), no_argument, 0, _T('h') },

        { 0, 0, 0, 0 }
    };

    int curropt = 0;
    BOOL bExitAfterOpt = FALSE;
    LPTSTR filter = NULL;
    LPTSTR ctx = NULL;

    //
    // Default options
    //
    LogSetLogLevel(LOG_ALL_TYPES, DEFAULT_OPT_DEBUGLEVEL);
    opt->misc.progression = DEFAULT_OPT_PROGRESSION;

    //
    // Parsing
    //
    while ((curropt = getopt_long_only(argc, argv, EMPTY_STR, long_options, NULL)) != -1) {
        switch (curropt) {
            //
            // List
            //
        case _T('L'):
            LogSetLogLevel(LOG_ALL_TYPES, _T("NONE"));
            ListImporters();
            ListFilters();
            ListWriters();
            ExitProcess(EXIT_SUCCESS);
            break;

        case _T('I'):
            LogSetLogLevel(LOG_ALL_TYPES, _T("NONE"));
            ListImporters();
            bExitAfterOpt = TRUE;
            break;

        case _T('F'):
            LogSetLogLevel(LOG_ALL_TYPES, _T("NONE"));
            ListFilters();
            bExitAfterOpt = TRUE;
            break;

        case _T('W'):
            LogSetLogLevel(LOG_ALL_TYPES, _T("NONE"));
            ListWriters();
            bExitAfterOpt = TRUE;
            break;

            //
            // Plugins
            //
        case _T('i'):
            opt->names.importers = optarg;
            opt->plugins.numberOfImporters = PluginLoadImporters(opt->names.importers, opt->plugins.importers, PLUGIN_MAX_IMPORTERS);
            break;

        case _T('f'):
            opt->names.filters = optarg;
            opt->plugins.numberOfFilters = PluginLoadFilters(opt->names.filters, opt->plugins.filters, PLUGIN_MAX_FILTERS);
            break;

        case _T('w'):
            opt->names.writers = optarg;
            opt->plugins.numberOfWriters = PluginLoadWriters(opt->names.writers, opt->plugins.writers, PLUGIN_MAX_WRITERS);
            break;

        case _T('r'):
            opt->names.inverted = optarg;

            DWORD i = 0;
            filter = NULL;
            ctx = NULL;

            // TODO : invert after filters (numOfF == 0)

            while (StrNextToken(opt->names.inverted, _T(","), &ctx, &filter)) {
                for (i = 0; i < PLUGIN_MAX_FILTERS; i++) {
                    if (PLUGIN_IS_LOADED(&opt->plugins.filters[i])) {
                        if (STR_EQ(filter, PLUGIN_GET_NAME(&opt->plugins.filters[i]))) {
                            LOG(Info, SUB_LOG(_T("Inverting filter <%s>")), filter);
                            opt->plugins.filters[i].inverted = TRUE;
                            break;
                        }
                    }
                }
                if (i == PLUGIN_MAX_FILTERS) {
                    FATAL(_T("Trying to invert a non-loaded filter : <%s>"), filter);
                }
            }
            break;

            //
            // Misc
            //
        case _T('p'):
            opt->misc.progression = _tstoi(optarg);
            LOG(Info, SUB_LOG(_T("Printing progression every <%u> ACE")), opt->misc.progression);
            break;

        case _T('l'):
            opt->misc.logfile = optarg;
            LogSetLogFile(opt->misc.logfile);
            break;

        case _T('d'):
            opt->misc.loglvl = optarg;
            LogSetLogLevel(LOG_ALL_TYPES, opt->misc.loglvl);
            break;

        case _T('h'):
            opt->misc.showHelp = TRUE;
            break;

        default:
            FATAL(_T("Unknown option"));
            break;
        }
    }


    LPTSTR optname = NULL;
    LPTSTR optval = NULL;
    int i = 0;
	HANDLE processHeap = NULL;

	processHeap = GetProcessHeap();
    for (i = optind; i < argc; i++) {
        ctx = NULL;

        StrNextToken(argv[i], _T("="), &ctx, &optname);
        optval = ctx; //We do not use StrNextToken here because optval can contain an equal sign.

        if (!optname || !optval) {
            FATAL(_T("Cannot parse plugin option <%s> (must be in format name=value)"), argv[i]);
        }
        else {
            LOG(Dbg, _T("Adding plugin option <%s : %s>"), optname, optval);
            AddStrPair((PUTILS_HEAP)&processHeap, &gs_PluginOptions.end, optname, optval);
            if (!gs_PluginOptions.head) {
                gs_PluginOptions.head = gs_PluginOptions.end;
            }
        }
    }

    if (bExitAfterOpt) {
        ExitProcess(EXIT_SUCCESS);
    }
}

static void ForeachLoadedPlugins(
    _In_ PACE_FILTER_OPTIONS options,
    _In_ PLUGIN_FN_GENERIC function
    ) {
    DWORD i = 0;

    for (i = 0; i < options->plugins.numberOfImporters; i++) {
        if (PLUGIN_IS_LOADED(&options->plugins.importers[i])) {
            function(PLUGIN_GENERIC(&options->plugins.importers[i]));
        }
    }

    for (i = 0; i < options->plugins.numberOfFilters; i++) {
        if (PLUGIN_IS_LOADED(&options->plugins.filters[i])) {
            function(PLUGIN_GENERIC(&options->plugins.filters[i]));
        }
    }

    for (i = 0; i < options->plugins.numberOfWriters; i++) {
        if (PLUGIN_IS_LOADED(&options->plugins.writers[i])) {
            function(PLUGIN_GENERIC(&options->plugins.writers[i]));
        }
    }
}

static BOOL PluginsRequires(
    _In_ PACE_FILTER_OPTIONS options,
    _In_ PLUGIN_REQUIREMENT req
    ) {
    DWORD i = 0;
    BOOL required = FALSE;

    // We dont care if the plugin is loaded, since its "requirements" will be zero if it's not.

    for (i = 0; i < options->plugins.numberOfImporters; i++) {
        required |= PLUGIN_REQUIRES(&options->plugins.importers[i], req);
    }
    for (i = 0; i < options->plugins.numberOfFilters; i++) {
        required |= PLUGIN_REQUIRES(&options->plugins.filters[i], req);
    }
    for (i = 0; i < options->plugins.numberOfWriters; i++) {
        required |= PLUGIN_REQUIRES(&options->plugins.writers[i], req);
    }

    return required;
}

static void ConstructAdminSdHolderSD(
    _In_ PPLUGIN_IMPORTER importer
    ) {
    DWORD count = 0;
    IMPORTED_ACE ace = { 0 };

    InitAdminSdHolderSd();
    importer->functions.ResetReading(&gc_PluginApiTable, ImporterAce);
    while (importer->functions.GetNextAce(&gc_PluginApiTable, &ace)) {
        if (ace.imported.objectDn && _tcsncmp(ADMIN_SD_HOLDER_PARTIAL_DN, ace.imported.objectDn, _tcslen(ADMIN_SD_HOLDER_PARTIAL_DN)) == 0) {
            AddAceToAdminSdHolderSd(&ace);
            count++;
        }
		importer->functions.FreeAce(&gc_PluginApiTable, &ace);
    }
    LOG(Info, SUB_LOG(_T("AdminSdHolder's DACL count : <%u>")), count);
}

static void ConstructObjectCache(
    _In_ PPLUGIN_IMPORTER importer
    ) {
    IMPORTED_OBJECT obj = { 0 };
    DWORD objCount = 0;

    if (!importer->functions.GetNextObject) {
        FATAL(_T("Object cache is required but importer <%s> does not allows Object importation"), PLUGIN_GET_NAME(importer));
    }
    importer->functions.ResetReading(&gc_PluginApiTable, ImporterObject);
    while (importer->functions.GetNextObject(&gc_PluginApiTable, &obj)) {
        obj.computed.number = ++objCount;
        CacheInsertObject(&obj);
		importer->functions.FreeObject(&gc_PluginApiTable, &obj);
    }
    CacheActivateObjectCache();
}

static void ConstructSchemaCache(
    _In_ PPLUGIN_IMPORTER importer
    ) {
    IMPORTED_SCHEMA sch = { 0 };
    DWORD schCount = 0;

    if (!importer->functions.GetNextSchema) {
        FATAL(_T("Schema cache is required but importer <%s> does allows Schema importation"), PLUGIN_GET_NAME(importer));
    }
    importer->functions.ResetReading(&gc_PluginApiTable, ImporterSchema);
    while (importer->functions.GetNextSchema(&gc_PluginApiTable, &sch)) {
        sch.computed.number = ++schCount;
        CacheInsertSchema(&sch);
		importer->functions.FreeSchema(&gc_PluginApiTable, &sch);
    }
    CacheActivateSchemaCache();
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
LPTSTR GetPluginOption(
    _In_ LPTSTR name,
    _In_ BOOL required
    ) {
    LPTSTR val = GetStrPair(gs_PluginOptions.head, name);
    if (!val) {
        if (required)
            FATAL(_T("Required plugin option <%s> is not present"), name);
    }
    LOG(Dbg, _T("Plugin option <%s : %s>"), name, val);
    return val;
}

int _tmain(
    _In_ int argc,
    _In_ TCHAR * argv[]
    ) {
    DWORD i = 0;
	HANDLE processHeap = NULL;

	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	LOG(Succ, _T("Starting"));
	processHeap = GetProcessHeap();
    //
    // Options parsing
    //
    LOG(Succ, _T("Parsing options"));
    ACE_FILTER_OPTIONS options = { 0 };
    DWORD importerAce = PLUGIN_MAX_IMPORTERS;
    DWORD importerObj = PLUGIN_MAX_IMPORTERS;
    DWORD importerSch = PLUGIN_MAX_IMPORTERS;

    if (argc > 1) {
        ParseOptions(&options, argc, argv);
    }
    else {
        options.misc.showHelp = TRUE;
    }

    if (options.misc.showHelp) {
        Usage(argv[0], NULL);
        ForeachLoadedPlugins(&options, PluginHelp);
        ExitProcess(EXIT_FAILURE);
    }
    //
    // Plugins verifications
    //
    LOG(Succ, _T("Verifying and choosing plugins"));

    if (options.plugins.numberOfImporters == 0){
        FATAL(_T("No importer has been loaded"));
    }

    if (options.plugins.numberOfWriters == 0){
        FATAL(_T("No writer has been loaded"));
    }

    for (i = 0; i < options.plugins.numberOfImporters; i++) {
        if (!PLUGIN_IS_LOADED(&options.plugins.importers[i])) {
            FATAL(_T("Importer <%u> is registered, but not loaded"), i);
        }

        if (importerAce == PLUGIN_MAX_IMPORTERS && options.plugins.importers[i].functions.GetNextAce) {
            LOG(Info, SUB_LOG(_T("Using <%s> to import ACE")), PLUGIN_GET_NAME(&options.plugins.importers[i]));
            importerAce = i;
        }

        if (importerObj == PLUGIN_MAX_IMPORTERS && options.plugins.importers[i].functions.GetNextSchema) {
            LOG(Info, SUB_LOG(_T("Using <%s> to import objects")), PLUGIN_GET_NAME(&options.plugins.importers[i]));
            importerObj = i;
        }

        if (importerSch == PLUGIN_MAX_IMPORTERS && options.plugins.importers[i].functions.GetNextObject) {
            LOG(Info, SUB_LOG(_T("Using <%s> to import schema")), PLUGIN_GET_NAME(&options.plugins.importers[i]));
            importerSch = i;
        }
    }

	if (importerAce == PLUGIN_MAX_IMPORTERS){
		FATAL(_T("ACE importer is missing"));
	}
	if (importerObj == PLUGIN_MAX_IMPORTERS){
		FATAL(_T("Obj importer is missing"));
	}
	if (importerSch == PLUGIN_MAX_IMPORTERS){
		FATAL(_T("Sch importer is missing"));
	}

    // We allow no filter to be loaded, to permit ACE format conversion (direct link importer->writer)
    if (options.plugins.numberOfFilters > 0) {
        for (i = 0; i < options.plugins.numberOfFilters; i++) {
            if (!PLUGIN_IS_LOADED(&options.plugins.filters[i])) {
                FATAL(_T("Filter <%u> is registered, but not loaded"), i);
            }
        }
    }

    for (i = 0; i < options.plugins.numberOfWriters; i++) {
        if (!PLUGIN_IS_LOADED(&options.plugins.writers[i])) {
            FATAL(_T("Writer <%u> is registered, but not loaded"), i);
        }
    }


    //
    // Initializing plugins
    //
    LOG(Succ, _T("Initializing plugins"));
    ForeachLoadedPlugins(&options, PluginInitialize);
    //
    // Constructing caches
    //
    LOG(Succ, _T("Constructing caches"));

    CachesInitialize();

    if (PluginsRequires(&options, OPT_REQ_SID_RESOLUTION) || PluginsRequires(&options, OPT_REQ_DN_RESOLUTION)) {
        LOG(Info, SUB_LOG(_T("Plugins require SID or DN resolution, constructing object cache")));
        ConstructObjectCache(&options.plugins.importers[importerObj]);
        LOG(Info, SUB_LOG(_T("Object cache count : <by-sid:%u> <by-dn:%u>")), CacheObjectBySidCount(), CacheObjectByDnCount());
    }

	if (PluginsRequires(&options, OPT_REQ_GUID_RESOLUTION) || PluginsRequires(&options, OPT_REQ_DISPLAYNAME_RESOLUTION)) {
        LOG(Info, SUB_LOG(_T("Plugins require GUID or DisplayName resolution, constructing schema cache")));
        ConstructSchemaCache(&options.plugins.importers[importerSch]);
        LOG(Info, SUB_LOG(_T("Schema cache count : <by-guid:%u> <by-displayname:%u>")), CacheSchemaByGuidCount(), CacheSchemaByDisplayNameCount());
    }

    if (PluginsRequires(&options, OPT_REQ_ADMINSDHOLDER_SD)) {
        LOG(Info, SUB_LOG(_T("Plugins require AdminSdHolder security descriptor, constructing it")));
        ConstructAdminSdHolderSD(&options.plugins.importers[importerAce]);
    }


    //
    // Main Loop : process and filter ACEs
    //
    LOG(Succ, _T("Starting ACE filtering"));

    IMPORTED_ACE ace = { 0 };
    BOOL passedFilters = TRUE;
    BOOL filterRet = FALSE;

    // Stats variables
    DWORD aceCount = 0;
    DWORD keptAceCount = 0;
    ULONGLONG timeStart = 0;

    timeStart = GetTickCount64();

    options.plugins.importers[importerAce].functions.ResetReading(&gc_PluginApiTable, ImporterAce);
    while (options.plugins.importers[importerAce].functions.GetNextAce(&gc_PluginApiTable, &ace)) {
        passedFilters = TRUE;
        ace.computed.number = ++aceCount;

        for (i = 0; i < options.plugins.numberOfFilters; i++) {

            filterRet = options.plugins.filters[i].functions.FilterAce(&gc_PluginApiTable, &ace);
            passedFilters &= options.plugins.filters[i].inverted ? !filterRet : filterRet;

            if (!passedFilters) {
                LOG(All, _T("Ace <%u> was filtered by <%s>"), aceCount, PLUGIN_GET_NAME(&options.plugins.filters[i]));
                break;
            }
        }

        if (passedFilters) {
            keptAceCount++;
            LOG(All, _T("Ace <%u> passed all filters"), aceCount);
            for (i = 0; i < options.plugins.numberOfWriters; i++) {
                options.plugins.writers[i].functions.WriteAce(&gc_PluginApiTable, &ace);
            }
        }
        if (aceCount % options.misc.progression == 0) {
            LOG(Info, SUB_LOG(_T("Count : %u")), aceCount);
        }
		options.plugins.importers[importerAce].functions.FreeAce(&gc_PluginApiTable, &ace);
    }

    //
    // Stats
    //
    LOG(Succ, _T("Done. ACE Statistics :"));
    LOG(Succ, SUB_LOG(_T("<total    : %u>")), aceCount);
    LOG(Succ, SUB_LOG(_T("<filtered : %06.2f%%  %u>")), PERCENT((aceCount - keptAceCount), aceCount), (aceCount - keptAceCount));
    LOG(Succ, SUB_LOG(_T("<kept     : %06.2f%%  %u>")), PERCENT(keptAceCount, aceCount), keptAceCount);
    LOG(Succ, SUB_LOG(_T("<time     : %.3fs>")), TIME_DIFF_SEC(timeStart, GetTickCount64()));


    //
    // Finalizing and unloading plugins
    //
    LOG(Succ, _T("Finalizing and unloading plugins"));
    ForeachLoadedPlugins(&options, PluginFinalize);
    ForeachLoadedPlugins(&options, PluginUnload);

    CachesDestroy();
    DestroyStrPairList((PUTILS_HEAP)&processHeap, gs_PluginOptions.head);

    //
    // End
    //
    LOG(Succ, _T("Done"));

	//
	// Cleanup
	//
	//WPP_CLEANUP();
	UtilsLibCleanup();
	CacheLibCleanup();
	CsvLibCleanup();
	LogLibCleanup();
    return EXIT_SUCCESS;
}
