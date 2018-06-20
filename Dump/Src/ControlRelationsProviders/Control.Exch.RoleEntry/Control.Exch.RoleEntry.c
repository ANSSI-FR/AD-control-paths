/* --- INCLUDES ------------------------------------------------------------- */
#include "..\Utils\Control.h"


/* --- DEFINES -------------------------------------------------------------- */
#define CONTROL_EXCH_ROLE_ENTRY_OUTFILE     _T("control.exch.roleentry.csv")
#define CONTROL_EXCH_ROLE_ENTRY_KEYWORD     _T("EXCH_ROLE_ENTRY")
#define CONTROL_ROLE_ENTRY_COUNT			(11)
#define ETS_PARTIAL_DN						_T("cn=exchange trusted subsystem,ou=microsoft exchange security groups,")


/* --- TYPES ---------------------------------------------------- */
typedef enum _ROLE_ENTRY_KEYWORD {
	RBAC_ADD_ADPERM = __COUNTER__,
	RBAC_ADD_MBXPERM = __COUNTER__,
	RBAC_SET_MBXFOLDERPERM = __COUNTER__,
	RBAC_ADD_MBXFOLDERPERM = __COUNTER__,
	RBAC_SET_MBX = __COUNTER__,
	RBAC_CONNECT_MBX = __COUNTER__,
	RBAC_ADD_ROLEGROUPMEMBER = __COUNTER__,
	RBAC_ADD_MGMTROLEENTRY = __COUNTER__,
	RBAC_ADD_RECIPIENTPERM = __COUNTER__,
	RBAC_NEW_MBXEXPORTREQ = __COUNTER__,
	RBAC_NEW_AUTHSERVER = __COUNTER__
} ROLE_ENTRY_KEYWORD;

static_assert(__COUNTER__ == CONTROL_ROLE_ENTRY_COUNT, "inconsistent value for CONTROL_ROLE_ENTRY_COUNT");


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
LPTSTR exchangeTrustedSubsystemDN = NULL;
LPTSTR controlRoleEntryList[] = {_T("add-adpermission"), _T("add-mailboxpermission"), _T("set-mailboxfolderpermission"),
_T("add-mailboxfolderpermission"), _T("set-mailbox"), _T("connect-mailbox"),
_T("add-rolegroupmember"), _T("add-managementroleentry"), _T("add-recipientpermission"),
_T("new-mailboxexportrequest"), _T("new-authserver")
};



/* --- PUBLIC VARIABLES ----------------------------------------------------- */
// This table must be consistent with the "ACE_RELATIONS" enum (same count, same order)
const LPTSTR gc_RoleEntryKeyword[CONTROL_ROLE_ENTRY_COUNT] = {
	STR(RBAC_ADD_ADPERM),
	STR(RBAC_ADD_MBXPERM),
	STR(RBAC_SET_MBXFOLDERPERM),
	STR(RBAC_ADD_MBXFOLDERPERM),
	STR(RBAC_SET_MBX),
	STR(RBAC_CONNECT_MBX),
	STR(RBAC_ADD_ROLEGROUPMEMBER),
	STR(RBAC_ADD_MGMTROLEENTRY),
	STR(RBAC_ADD_RECIPIENTPERM),
	STR(RBAC_NEW_MBXEXPORTREQ),
	STR(RBAC_NEW_AUTHSERVER)
};

static_assert(ARRAY_COUNT(gc_RoleEntryKeyword) == CONTROL_ROLE_ENTRY_COUNT, "inconsistent number of keywords in gc_RoleEntryKeyword");


/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
static void CallbackExchRoleEntry(
	_In_ CSV_HANDLE hOutfile,
	_In_ CSV_HANDLE hDenyOutfile,
	_Inout_ LPTSTR *tokens
) {
	UNREFERENCED_PARAMETER(hDenyOutfile);
	BOOL bResult = FALSE;
	LPTSTR roleEntry = NULL;
	LPTSTR roleEntryName = NULL;
	LPTSTR listMsExchRoleEntries = NULL;
	LPTSTR nextEntry = NULL;
	LPTSTR nextEntryField = NULL;
	LPTSTR domainDN = NULL;
	LPTSTR dn = NULL;
	DWORD roleEntryIndex = 0;
	size_t eTSDNlen = 0;

	if (!exchangeTrustedSubsystemDN) {
		dn = _tcsdup(tokens[LdpListDn]);
		domainDN = _tcsstr(dn, _T("dc="));
		eTSDNlen = _tcslen(ETS_PARTIAL_DN) + _tcslen(domainDN) + 1;
		exchangeTrustedSubsystemDN = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,eTSDNlen * sizeof(TCHAR));
		if (!exchangeTrustedSubsystemDN) {
			LOG(Err, _T("Cannot allocate DN string"));
		}
		_tcsncat_s(exchangeTrustedSubsystemDN, eTSDNlen, ETS_PARTIAL_DN, _tcslen(ETS_PARTIAL_DN));
		_tcsncat_s(exchangeTrustedSubsystemDN, eTSDNlen, domainDN, _tcslen(domainDN));
	}

	// Do we have role entries
	if (STR_EMPTY(tokens[LdpListMsExchRoleEntries]))
		return;

	listMsExchRoleEntries = _tcsdup(tokens[LdpListMsExchRoleEntries]);
	CharLower(listMsExchRoleEntries);
	roleEntry = _tcstok_s(listMsExchRoleEntries, _T(";"), &nextEntry);
	while (roleEntry) {
		roleEntryName = _tcstok_s(roleEntry, _T(","), &nextEntryField);
		roleEntryName = _tcstok_s(NULL, _T(","), &nextEntryField);
		if (IsInSetOfStrings(roleEntryName, controlRoleEntryList, CONTROL_ROLE_ENTRY_COUNT, &roleEntryIndex)) {
			bResult = ControlWriteOutline(hOutfile, tokens[LdpListDn], exchangeTrustedSubsystemDN, gc_RoleEntryKeyword[roleEntryIndex]);
			if (!bResult) {
				LOG(Err, _T("Cannot write outline for <%s>"), tokens[LdpListDn]);
			}
		}
		nextEntryField = NULL;
		roleEntry = _tcstok_s(NULL, _T(";"), &nextEntry);
	}
	free(listMsExchRoleEntries);
}



/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
int _tmain(
	_In_ int argc,
	_In_ TCHAR * argv[]
) {
	//
	// Init
	//
	//WPP_INIT_TRACING();
	UtilsLibInit();
	CacheLibInit();
	CsvLibInit();
	LogLibInit();

	PTCHAR outfileHeader[OUTFILE_TOKEN_COUNT] = CONTROL_OUTFILE_HEADER;
	bCacheBuilt = FALSE;
	ControlMainForeachCsvResult(argc, argv, outfileHeader, CallbackExchRoleEntry, GenericUsage);

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
