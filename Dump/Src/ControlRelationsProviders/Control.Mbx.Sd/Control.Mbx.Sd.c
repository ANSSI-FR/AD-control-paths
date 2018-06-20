/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_MBX_SD_DEFAULT_OUTFILE       _T("control.mbx.sd.csv")
#define CONTROL_MBX_OWNER_KEYWORD            _T("MBXSD_OWNER")
#define CONTROL_MBX_NULL_DACL_KEYWORD        _T("MBXSD_NULL_DACL")


/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
void CallbackBuildCaches(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_In_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hOutfile);
	UNREFERENCED_PARAMETER(hDenyOutfile);

	BOOL bResult = FALSE;
	CACHE_OBJECT_BY_SID cacheEntry = { 0 };
	PCACHE_OBJECT_BY_SID inserted = NULL;
	CACHE_MAIL_BY_DN mbxCacheEntry = { 0 };
	PCACHE_MAIL_BY_DN insertedMbx = NULL;
	BOOL newElement = FALSE;
	PSID pSid = NULL;

	// Build Sid Cache
	if (STR_EMPTY(tokens[LdpListObjectSid]))
		return;

	LOG(Dbg, _T("Object <%s> has hex SID <%s>"), tokens[LdpListDn], tokens[LdpListObjectSid]);

	pSid = (PSID)malloc(SECURITY_MAX_SID_SIZE);
	if (!pSid)
		FATAL(_T("Could not allocate SID <%s>"), tokens[LdpListObjectSid]);

	if (_tcslen(tokens[LdpListObjectSid]) / 2 > SECURITY_MAX_SID_SIZE) {
		FATAL(_T("Hex sid <%s> too long"), tokens[LdpListObjectSid]);
	}
	Unhexify(pSid, tokens[LdpListObjectSid]);
	bResult = IsValidSid(pSid);
	if (!bResult) {
		FATAL(_T("Invalid SID <%s> for <%s>"), tokens[LdpListObjectSid], tokens[LdpListDn]);
	}

	ConvertSidToStringSid(pSid, &cacheEntry.sid);
	free(pSid);
	cacheEntry.dn = malloc((_tcslen(tokens[LdpListDn]) + 1) * sizeof(TCHAR));
	if (!cacheEntry.dn)
		FATAL(_T("Could not allocate dc <%s>"), tokens[LdpListDn]);
	_tcscpy_s(cacheEntry.dn, _tcslen(tokens[LdpListDn]) + 1, tokens[LdpListDn]);

	CacheEntryInsert(
		ppCache,
		(PVOID)&cacheEntry,
		sizeof(CACHE_OBJECT_BY_SID),
		&inserted,
		&newElement
	);

	if (!inserted) {
		LOG(Err, _T("cannot insert new object-by-sid cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-sid cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	//Build Mbx cache
	if (STR_EMPTY(tokens[LdpListMail]))
		return;

	LOG(Dbg, _T("Object <%s> has mail <%s>"), tokens[LdpListDn], tokens[LdpListMail]);
	mbxCacheEntry.dn = cacheEntry.dn;
	mbxCacheEntry.mail = malloc((_tcslen(tokens[LdpListMail]) + 1) * sizeof(TCHAR));
	if (!mbxCacheEntry.mail)
		FATAL(_T("Could not allocate <%s>"), tokens[LdpListMail]);
	_tcscpy_s(mbxCacheEntry.mail, _tcslen(tokens[LdpListMail]) + 1, tokens[LdpListMail]);

	CacheEntryInsert(
		ppMbxCache,
		(PVOID)&mbxCacheEntry,
		sizeof(CACHE_MAIL_BY_DN),
		&insertedMbx,
		&newElement
	);

	if (!insertedMbx) {
		LOG(Err, _T("cannot insert new mail-by-dn cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-sid cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	return;
}




static void CallbackMbxOwner(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PSECURITY_DESCRIPTOR pSd;
	BOOL bOwnerDefaulted = FALSE;
	PSID pSidOwner = NULL;
	CACHE_OBJECT_BY_SID searched = { 0 };
	PCACHE_OBJECT_BY_SID returned = NULL;
	CACHE_MAIL_BY_DN searchedMbx = { 0 };
	PCACHE_MAIL_BY_DN returnedMbx = NULL;
	LPTSTR owner = NULL;
	LPTSTR mail = NULL;

	UNREFERENCED_PARAMETER(hDenyOutfile);

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
		LOG(Dbg, _T("No Owner from SD : <%s>"), tokens[LdpAceDn]);
		return;
	}

	ConvertSidToStringSid(pSidOwner, &searched.sid);
	// Owner is SELF
	if (!_tcsicmp(_T("S-1-5-10"), searched.sid)) {
		owner = tokens[LdpAceDn];
	}
	else {
		CharLower(searched.sid);
		bResult = CacheEntryLookup(
			ppCache,
			(PVOID)&searched,
			&returned
		);
		if (!returned) {
			LOG(Dbg, _T("cannot find object-by-sid entry for SD owner of <%s>"), tokens[LdpAceDn]);
			owner = searched.sid;
		}
		else {
			owner = returned->dn;
		}
	}
	LocalFree(searched.sid);
	free(pSd);

	//Look for mail address of dn
	searchedMbx.dn = tokens[LdpAceDn];
	bResult = CacheEntryLookup(
		ppMbxCache,
		(PVOID)&searchedMbx,
		&returnedMbx
	);
	if (!returnedMbx) {
		LOG(Dbg, _T("cannot find mail-by-dn entry for SD owner of <%s>"), tokens[LdpAceDn]);
		mail = searchedMbx.dn;
	}
	else {
		mail = returnedMbx->mail;
	}
	bResult = ControlWriteOutline(hOutfile, owner, mail, CONTROL_MBX_OWNER_KEYWORD);
	if (!bResult) {
		LOG(Err, _T("Cannot write outline for <%s>"), mail);
	}

	return;
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptNameSid = _T("SIDCACHE");
	PTCHAR ptNameMbx = _T("MBXCACHE");
	bCacheBuilt = FALSE;
	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	CacheCreate(
		&ppCache,
		ptNameSid,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	CacheCreate(
		&ppMbxCache,
		ptNameMbx,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildCaches, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackMbxOwner, GenericUsage);

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
