/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_SD_DEFAULT_OUTFILE       _T("control.ad.sd.csv")
#define CONTROL_AD_OWNER_KEYWORD            _T("AD_OWNER")
#define CONTROL_AD_NULL_DACL_KEYWORD        _T("AD_NULL_DACL")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PTCHAR gs_ptSidEveryone = NULL;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackSdOwner(
	_In_ CSV_HANDLE hOutfile,
	_Inout_ LPTSTR *tokens
    ) {
    BOOL bResult = FALSE;
    PSECURITY_DESCRIPTOR pSd;
	BOOL bOwnerDefaulted = FALSE;
	PSID pSidOwner = NULL;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;

	if (STR_EMPTY(tokens[LdpAceSd]))
		return;

	pSd = malloc((_tcslen(tokens[LdpAceSd]) * sizeof(TCHAR)) / 2);
	if (!pSd)
		FATAL(_T("Cannot allocate pSd for %s"), tokens[LdpAceSd]);
	Unhexify(pSd, tokens[LdpAceSd]);
    if (!IsValidSecurityDescriptor(pSd)) {
        LOG(Err, _T("Invalid security descriptor"));
        return;
    }
	bResult = GetSecurityDescriptorOwner(pSd, &pSidOwner, &bOwnerDefaulted);
	if (!bResult) {
		LOG(Err, _T("Cannot get Owner from SD : <%u>"), GetLastError());
		return;
	}
	if (!pSidOwner) {
		LOG(Dbg, _T("No Owner from SD : <%u>"), tokens[LdpAceDn]);
		return;
	}
	ConvertSidToStringSid(pSidOwner, &searched.sid);
	bResult = CacheEntryLookup(
		ppCache,
		(PVOID)&searched,
		&returned
	);
	LocalFree(searched.sid);
	if (!returned) {
		LOG(Dbg, _T("cannot find object-by-sid entry for <%d>"), tokens[LdpListPrimaryGroupID]);
		return;
	}

	bResult = ControlWriteOutline(hOutfile, returned->dn, tokens[LdpAceDn], CONTROL_AD_OWNER_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
	}
	free(pSd);
	return;
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptName = _T("SIDCACHE");
	bCacheBuilt = FALSE;
	CacheCreate(
		&ppCache,
		ptName,
		pfnCompare
	);
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildSidCache, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackSdOwner, GenericUsage);

	return EXIT_SUCCESS;
}