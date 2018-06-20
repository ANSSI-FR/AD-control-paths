/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"
#include <NtDsAPI.h>


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_AD_DELEG_DEFAULT_OUTFILE       _T("control.ad.deleg.csv")
#define CONTROL_AD_DELEG_KEYWORD               _T("KRB_DELEG")
#define CONTROL_AD_DELEG_RSRC_KEYWORD          _T("KRB_RSRC_DELEG")



/* --- TYPES ---------------------------------------------------- */
/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PTCHAR gs_domainNC = NULL;


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
	CACHE_OBJECT_BY_SID sidCacheEntry = { 0 };
	PCACHE_OBJECT_BY_SID insertedSid = NULL;
	CACHE_OBJECT_BY_SPN spnCacheEntry = { 0 };
	PCACHE_OBJECT_BY_SPN insertedSpn = NULL;
	CACHE_OBJECT_BY_DNSHOSTNAME dnsCacheEntry = { 0 };
	PCACHE_OBJECT_BY_DNSHOSTNAME insertedDns = NULL;
	BOOL newElement = FALSE;
	LPTSTR ctx = NULL;
	LPTSTR SPNItem = NULL;
	PSID pSid = NULL;

	if (gs_recordNumber == 1)
		gs_domainNC = _tcsdup(tokens[0]);

	// Build Sid Cache
	if (!STR_EMPTY(tokens[LdpListObjectSid]))
	{

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

	ConvertSidToStringSid(pSid, &sidCacheEntry.sid);
	free(pSid);
	sidCacheEntry.dn = malloc((_tcslen(tokens[LdpListDn]) + 1) * sizeof(TCHAR));
	if (!sidCacheEntry.dn)
		FATAL(_T("Could not allocate dc <%s>"), tokens[LdpListDn]);
	_tcscpy_s(sidCacheEntry.dn, _tcslen(tokens[LdpListDn]) + 1, tokens[LdpListDn]);

	CacheEntryInsert(
		ppCache,
		(PVOID)&sidCacheEntry,
		sizeof(CACHE_OBJECT_BY_SID),
		&insertedSid,
		&newElement
	);

	if (!insertedSid) {
		LOG(Err, _T("cannot insert new object-by-sid cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-sid cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	
	}
	
	//Build SPN cache
	if (!STR_EMPTY(tokens[LdpListSPN]))
	{
	LOG(Dbg, _T("Object <%s> has SPNs <%s>"), tokens[LdpListDn], tokens[LdpListSPN]);
	//parse csv list
	while (StrNextToken(tokens[LdpListSPN],_T(";"), &ctx, &SPNItem)) {
		spnCacheEntry.dn = sidCacheEntry.dn;
		spnCacheEntry.spn = malloc((_tcslen(SPNItem) + 1) * sizeof(TCHAR));
		if (!spnCacheEntry.spn)
			FATAL(_T("Could not allocate <%s>"), SPNItem);
		_tcscpy_s(spnCacheEntry.spn, _tcslen(SPNItem) + 1, SPNItem);

	CacheEntryInsert(
		ppSpnCache,
		(PVOID)&spnCacheEntry,
		sizeof(CACHE_OBJECT_BY_SPN),
		&insertedSpn,
		&newElement
	);

	if (!insertedSpn) {
		LOG(Err, _T("cannot insert new object-by-spn cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-spn cache entry is not new <%s>"), tokens[LdpListDn]);
	}
	//end while
	}
	
	}
	
	//Build dNSHostName cache
	if (!STR_EMPTY(tokens[LdpListDnsHostName]))
	{	

	LOG(Dbg, _T("Object <%s> has dNSHostName <%s>"), tokens[LdpListDn], tokens[LdpListDnsHostName]);
	dnsCacheEntry.dn = sidCacheEntry.dn;
	dnsCacheEntry.dnshostname = malloc((_tcslen(tokens[LdpListDnsHostName]) + 1) * sizeof(TCHAR));
	if (!dnsCacheEntry.dnshostname)
		FATAL(_T("Could not allocate <%s>"), tokens[LdpListDnsHostName]);
	_tcscpy_s(dnsCacheEntry.dnshostname, _tcslen(tokens[LdpListDnsHostName]) + 1, tokens[LdpListDnsHostName]);

	CacheEntryInsert(
		ppDnsCache,
		(PVOID)&dnsCacheEntry,
		sizeof(CACHE_OBJECT_BY_DNSHOSTNAME),
		&insertedDns,
		&newElement
	);

	if (!insertedDns) {
		LOG(Err, _T("cannot insert new object-by-dnshostname cache entry <%s>"), tokens[LdpListDn]);
	}
	else if (!newElement) {
		LOG(Dbg, _T("object-by-dnshostname entry is not new <%s>"), tokens[LdpListDn]);
	}
	
	}
	
}


static void CallbackKrbDeleg(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	BOOL bResult = FALSE;
	PSECURITY_DESCRIPTOR pSd;
	BOOL daclPresent = FALSE;
	BOOL daclDefaulted = FALSE;
	PACL dacl = NULL;
	ACL_SIZE_INFORMATION acl_size_info = { 0 };
	PACCESS_ALLOWED_ACE ace = NULL;
	LPTSTR trustee = NULL;
	
	CACHE_OBJECT_BY_SID searchedbysid = { 0 };
	PCACHE_OBJECT_BY_SID returnedbysid = NULL;
	CACHE_OBJECT_BY_SPN searchedbyspn = { 0 };
	PCACHE_OBJECT_BY_SPN returnedbyspn = NULL;
	CACHE_OBJECT_BY_DNSHOSTNAME searchedbydns = { 0 };
	PCACHE_OBJECT_BY_DNSHOSTNAME returnedbydns = NULL;
	LPTSTR instanceName = NULL;
	DWORD pcInstanceName = 0;
	TCHAR   szTemp[1];
	LPTSTR ctx = NULL;
	LPTSTR delegItem = NULL;
	int UAC = 0;

	UNREFERENCED_PARAMETER(hDenyOutfile);

	if (!STR_EMPTY(tokens[LdpListUAC]))
	{
		UAC = _tstoi(tokens[LdpListUAC]);
		if (UAC & 0x80000 || UAC & 0x1000000)
			if (STR_EMPTY(tokens[LdpListAllowedToDelegateTo]))
				bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], gs_domainNC, CONTROL_AD_DELEG_KEYWORD);
			else
			{
				while (StrNextToken(tokens[LdpListAllowedToDelegateTo], _T(";"), &ctx, &delegItem)) {
					searchedbyspn.spn = delegItem;
					bResult = CacheEntryLookup(
						ppSpnCache,
						(PVOID)&searchedbyspn,
						&returnedbyspn
					);
					if (returnedbyspn) {
						bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], returnedbyspn->dn, CONTROL_AD_DELEG_KEYWORD);			
					}
					else {
						//Assuming computer account identity can rarely get false positives as it can be an invalid service class
						//Ideally we should check aliases in cn=directory service,cn=windows NT,cn=services,cn=configuration
						//Most cases will be aliases of HOST though (cifs, etc.)
						LOG(Dbg, _T("cannot find object-by-spn for allowedtodelegate of <%s>, assuming computer account identity"), tokens[LdpListDn]);
						DsCrackSpn(delegItem, NULL, NULL, NULL, NULL, &pcInstanceName, szTemp, NULL);
						instanceName = malloc(pcInstanceName * sizeof(TCHAR));
						bResult = DsCrackSpn(delegItem, NULL, NULL, NULL, NULL, &pcInstanceName, instanceName, NULL);
						if(!bResult)
							
						searchedbydns.dnshostname = instanceName;
						bResult = CacheEntryLookup(
							ppDnsCache,
							(PVOID)&searchedbydns,
							&returnedbydns
						);
						if (returnedbydns) {
							bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], returnedbydns->dn, CONTROL_AD_DELEG_KEYWORD);
						}
						else {
							LOG(Dbg, _T("cannot find object-by-hostname entry for allowedtodelegate of <%s>"), searchedbydns.dnshostname);
						}
						free(instanceName);
					}
				}
			}			
	}

	if (!STR_EMPTY(tokens[LdpListAllowedToActOnBehalf]))
	{
		pSd = malloc(_tcslen(tokens[LdpListAllowedToActOnBehalf]) / 2);
		if (!pSd)
			FATAL(_T("Cannot allocate pSd for %s"), tokens[LdpListAllowedToActOnBehalf]);
		Unhexify(pSd, tokens[LdpListAllowedToActOnBehalf]);
		if (!IsValidSecurityDescriptor(pSd)) {
			LOG(Err, _T("Invalid security descriptor"));
			return;
		}
		bResult = GetSecurityDescriptorDacl(pSd,&daclPresent,&dacl,&daclDefaulted);
		if (bResult && daclPresent && IsValidAcl(dacl))
			if (GetAclInformation(dacl,(PVOID)&acl_size_info, sizeof(acl_size_info), AclSizeInformation))
				for (DWORD i = 0; i < acl_size_info.AceCount; i++)
					if (GetAce(dacl, i, (PVOID)&ace)) {
						ConvertSidToStringSid((PSID)&(ace->SidStart), &searchedbysid.sid);
						CharLower(searchedbysid.sid);
						bResult = CacheEntryLookup(
						ppCache,
						(PVOID)&searchedbysid,
						&returnedbysid
						);
						if (!returnedbysid) {
							LOG(Dbg, _T("cannot find object-by-sid entry for rsrc deleg trustee of <%s>"), tokens[LdpAceDn]);
							trustee = searchedbysid.sid;
						}
						else {
							trustee = returnedbysid->dn;
						}
						bResult = ControlWriteOutline(hOutfile, trustee, tokens[LdpAceDn], CONTROL_AD_DELEG_RSRC_KEYWORD);
						if (!bResult) {
							LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpAceDn]);
						}
						LocalFree(searchedbysid.sid);
					}
		free(pSd);
	}		
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	PTCHAR ptNameSid = _T("SIDCACHE");
	PTCHAR ptNameSpn = _T("SPNCACHE");
	PTCHAR ptNameDns = _T("DNSCACHE");
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
		&ppSpnCache,
		ptNameSpn,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	CacheCreate(
		&ppDnsCache,
		ptNameDns,
		(PRTL_AVL_COMPARE_ROUTINE)pfnCompare
	);
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackBuildCaches, GenericUsage);
	bCacheBuilt = TRUE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackKrbDeleg, GenericUsage);

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
