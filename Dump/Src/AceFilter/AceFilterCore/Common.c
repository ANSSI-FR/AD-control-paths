/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "Common.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LOG_LEVEL gs_debugLevel = Succ;
static HANDLE gs_hLogFile = INVALID_HANDLE_VALUE;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void SetLogLevel(
    _In_ LPTSTR lvl
    ) {
    static const LPCTSTR gs_DebugLevelSets[] = { _T("ALL"), _T("DBG"), _T("INFO"), _T("WARN"), _T("ERR"), _T("SUCC"), _T("NONE") };
    DWORD index = gs_debugLevel;

    if (IsInSetOfStrings(lvl, gs_DebugLevelSets, ARRAY_COUNT(gs_DebugLevelSets), &index)) {
        gs_debugLevel = (LOG_LEVEL)index;
        LOG(Info, SUB_LOG(_T("Setting log level to <%s>")), lvl);
    }
    else {
        LOG(Err, SUB_LOG(_T("Unknown log level <%s>")), lvl);
    }
}

BOOL SetLogFile(
    _In_ LPTSTR logFile
    ) {
    gs_hLogFile = CreateFile(logFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (gs_hLogFile == INVALID_HANDLE_VALUE) {
        FATAL(SUB_LOG(_T("Failed to open log file <%s> : <%u>")), logFile, GetLastError());
    }

    LOG(Info, SUB_LOG(_T("Setting log file to <%s>")), logFile);

    return TRUE;
}

void Log(
    _In_  LOG_LEVEL   lvl,
    _In_  LPTSTR      frmt,
    _In_  ...
    ) {
    if (lvl >= gs_debugLevel) {
        TCHAR line[MAX_LINE] = { 0 };
        BOOL bResult = FALSE;
        va_list args;
        int size;
        size_t count;
        DWORD written;
        SYSTEMTIME time;

        GetSystemTime(&time);

        va_start(args, frmt);
        if (gs_hLogFile != INVALID_HANDLE_VALUE) {
            size = _stprintf_s(line, _countof(line), LOG_FRMT_TIME, time.wHour, time.wMinute, time.wSecond);
            if (size == -1) {
                _ftprintf(stderr, _T("Fatal error while formating log time. <errno:%x>"), errno);
                ExitProcess(EXIT_FAILURE);
            }

            bResult = WriteFile(gs_hLogFile, line, size, &written, NULL);
            if (!bResult || written != (DWORD)size) {
                _ftprintf(stderr, _T("Fatal error while writing log time to logfile : <gle:%u> <written:%u/%u>"), GetLastError(), written, (DWORD)size);
                ExitProcess(EXIT_FAILURE);
            }
            count = fwrite(line, size, 1, stderr);
            if (count != 1) {
                _ftprintf(stderr, _T("Fatal error while writing log time to output : <errno:%x>"), errno);
                ExitProcess(EXIT_FAILURE);
            }

            size = _vstprintf_s(line, _countof(line), frmt, args);
            if (size == -1) {
                _ftprintf(stderr, _T("Fatal error while formating log line. <errno:%x> <frmt:%s>"), errno, frmt);
                ExitProcess(EXIT_FAILURE);
            }
            bResult = WriteFile(gs_hLogFile, line, size, &written, NULL);
            if (!bResult || written != (DWORD)size) {
                _ftprintf(stderr, _T("Fatal error while writing log line to logfile : <gle:%u> <written:%u/%u> <frmt:%s>"), GetLastError(), written, (DWORD)size, frmt);
                ExitProcess(EXIT_FAILURE);
            }
            count = fwrite(line, size, 1, stderr);
            if (count != 1) {
                _ftprintf(stderr, _T("Fatal error while writing log line to output : <errno:%x> <count:%llu> <frmt:%s>"), errno, count, frmt);
                ExitProcess(EXIT_FAILURE);
            }
        }
        else {
            size = _ftprintf(stderr, LOG_FRMT_TIME, time.wHour, time.wMinute, time.wSecond);
            if (size == -1) {
                _ftprintf(stderr, _T("Fatal error while writing log time to output : <errno:%x>"), errno);
                ExitProcess(EXIT_FAILURE);
            }
            size = _vftprintf(stderr, frmt, args);
            if (size == -1) {
                _ftprintf(stderr, _T("Fatal error while writing log line to output : <errno:%x> <size:%u> <frmt:%s>"), errno, (DWORD)size, frmt);
                ExitProcess(EXIT_FAILURE);
            }
        }
        va_end(args);

        fflush(stderr);
    }
}

BOOL IsInSetOfStrings(
    _In_  LPCTSTR        arg,
    _In_  const LPCTSTR  stringSet[],
    _In_  DWORD          setSize,
    _Out_opt_ DWORD          *index
    ) {
    if (index) {
        *index = 0;
    }

    for (DWORD i = 0; i < setSize; i++) {
        if (_tcsicmp(stringSet[i], arg) == 0) {
            if (index) {
                *index = i;
            }
            return TRUE;
        }
    }

    return FALSE;
}

BOOL MemoryIsNull(
    _In_ PBYTE mem,
    _In_ DWORD len
    ) {
    DWORD i = 0;
    for (i = 0; i < len; i++) {
        if (mem[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL StrNextToken(
    _In_ LPTSTR str,
    _In_ LPTSTR del,
    _Inout_ LPTSTR *ctx,
    _Out_ LPTSTR *tok
    ) {
    if (*ctx == NULL) {
        *tok = _tcstok_s(str, del, ctx);
    }
    else {
        *tok = _tcstok_s(NULL, del, ctx);
    }
    return (*tok != NULL);
}

static BYTE ChartoInt(
    _In_ char c
    ) {
    if (c >= '0' && c <= '9') return      c - '0';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    return 0;
}

void Hexify(
    _Inout_ LPTSTR outstr,
    _In_ PBYTE indata,
    _In_ DWORD len // outstr must be able to receive len*2 chars + null
    ) {
    DWORD i = 0;
    const static CHAR conv[] = "0123456789ABCDEF";
    for (i = 0; i < len; i++) {
        outstr[(2 * i) + 0] = conv[((indata[i] & 0xF0) >> 4)];
        outstr[(2 * i) + 1] = conv[((indata[i] & 0x0F) >> 0)];
    }
    outstr[2 * len] = '\0';
}

void Unhexify(
    _Inout_ PBYTE outdata,
    _In_ LPTSTR instr // outdata must be able to receive strlen(instr)/2 bytes
    ) {
    DWORD i = 0;
    size_t len = _tcslen(instr) / 2;
    for (i = 0; i < len; i++) {
        outdata[i] = 16 * ChartoInt(instr[(2 * i) + 0]) + ChartoInt(instr[(2 * i) + 1]);
    }
}

void AddStrPair(
    _Inout_ PSTR_PAIR_LIST *end,
    _In_ LPTSTR name,
    _In_ LPTSTR value
    ) {
    PSTR_PAIR_LIST pair = (PSTR_PAIR_LIST)LocalAllocCheckX(sizeof(STR_PAIR_LIST));
    pair->next = NULL;
    pair->name = name;
    pair->val = value;

    if (*end) {
        (*end)->next = pair;
    }
    *end = pair;
}


LPTSTR GetStrPair(
    _In_ PSTR_PAIR_LIST head,
    _In_ LPTSTR name
    ) {
    PSTR_PAIR_LIST curr = head;

    while (curr) {
        if (STR_EQ(name, curr->name)) {
            return curr->val;
        }
        curr = curr->next;
    }

    return NULL;
}

void DestroyStrPairList(
    _In_ PSTR_PAIR_LIST head
    ) {
    PSTR_PAIR_LIST curr = head;
    PSTR_PAIR_LIST p = NULL;

    while (curr) {
        p = curr;
        curr = curr->next;
        LocalFreeCheckX(p);
    }
}

HLOCAL LocalAllocCheck(
    _In_  LPTSTR   szCallerName,
    _In_  SIZE_T   uBytes
    ) {
    HLOCAL mem = LocalAlloc(LMEM_ZEROINIT, uBytes);

    if (!mem) {
        FATAL(_T("[%s] cannot allocate <%u> bytes of memory (local) : <%u>"), szCallerName, uBytes, GetLastError());
    }

    return mem;
}

void LocalFreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HLOCAL hMem
    ) {
    HLOCAL mem = LocalFree(hMem);

    if (mem) {
        FATAL(_T("[%s] cannot free memory (local) : <%u>"), szCallerName, GetLastError());
    }
}

LPVOID HeapAllocCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HANDLE   hHeap,
    _In_  DWORD    dwFlags,
    _In_  SIZE_T   dwBytes
    ) {
    PVOID mem = HeapAlloc(hHeap, dwFlags, dwBytes);

    if (!mem) {
        FATAL(_T("[%s] cannot allocate <%u> bytes of heap <%08x> : <%u>"), szCallerName, dwBytes, hHeap, GetLastError());
    }

    return mem;
}

void HeapFreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_  HANDLE   hHeap,
    _In_  DWORD    dwFlags,
    _In_  LPVOID   lpMem
    ) {
    if (!HeapFree(hHeap, dwFlags, lpMem)) {
        FATAL(_T("[%s] cannot free memory <%p> of heap <%08x> : <%u>"), szCallerName, lpMem, hHeap, GetLastError());
    }
}

LPTSTR StrDupCheck(
    _In_  LPTSTR   szCallerName,
    _In_  LPTSTR   szStrToDup
    ) {
    LPTSTR szDupped = _tcsdup(szStrToDup);
    if (!szDupped) {
        FATAL(_T("[%s] cannot dup string <%s>"), szCallerName, szStrToDup);
    }
    return szDupped;
}

void FreeCheck(
    _In_  LPTSTR   szCallerName,
    _In_ void *memblock
    ) {
    free(memblock);
    if (errno != 0) {
        FATAL(_T("[%s] cannot free memory <%p> : <errno:%u>"), szCallerName, memblock, errno);
    }
}


// Function taken from another project by A.Bordes.
BOOL IsAceInSd(
    _In_ PACE_HEADER pHeaderAceToCheck,
    _In_ PSECURITY_DESCRIPTOR pSd
    ) {
    BOOL bResult, bDaclPresent, bDaclDefaulted;
    ACL_SIZE_INFORMATION AclSizeInfo;
    PACL pDacl;
    PACE_HEADER pAceHeader;

    // S'il n'y a pas de descripteur de sécurité, on renvoie tourjous FALSE
    if (!pSd)
        return FALSE;

    bResult = GetSecurityDescriptorDacl(pSd, &bDaclPresent, &pDacl, &bDaclDefaulted);
    if (!bResult || !bDaclPresent)
        return FALSE;

    bResult = GetAclInformation(pDacl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation);
    if (!bResult)
        return FALSE;

    // On récupère les informations
    for (DWORD i = 0; i < AclSizeInfo.AceCount; i++)
    {
        // On récupère l'ACE
        bResult = GetAce(pDacl, i, (LPVOID*)&pAceHeader);
        if (!bResult)
            return FALSE;

        if (pHeaderAceToCheck->AceType == pAceHeader->AceType)
        {
            if ((pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE) || (pAceHeader->AceType == ACCESS_DENIED_ACE_TYPE))
            {
                PACCESS_ALLOWED_ACE pAce, pAceToCheck;
                pAce = (PACCESS_ALLOWED_ACE)pAceHeader;
                pAceToCheck = (PACCESS_ALLOWED_ACE)pHeaderAceToCheck;

                if ((pAce->Mask == pAceToCheck->Mask)
                    && (EqualSid(&pAce->SidStart, &pAceToCheck->SidStart)))
                {
                    return TRUE;
                }
            }
            else if ((pAceHeader->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) || (pAceHeader->AceType == ACCESS_DENIED_OBJECT_ACE_TYPE))
            {
                PACCESS_ALLOWED_OBJECT_ACE pAce, pAceToCheck;
                pAce = (PACCESS_ALLOWED_OBJECT_ACE)pAceHeader;
                pAceToCheck = (PACCESS_ALLOWED_OBJECT_ACE)pHeaderAceToCheck;

                PSID pSidToCheck, pSidAce;

                if ((pAce->Flags == pAceToCheck->Flags) && (pAce->Mask == pAceToCheck->Mask))
                {
                    if (pAce->Flags == 0)
                    {
                        // MSDN (SidStart) : The offset of this member can vary.
                        // If the Flags member is zero, the SidStart member starts at the offset specified by the ObjectType member.
                        pSidToCheck = &pAceToCheck->ObjectType;
                        pSidAce = &pAce->ObjectType;

                        if (EqualSid(pSidToCheck, pSidAce))
                            return TRUE;
                    }
                    else if ((pAce->Flags == ACE_OBJECT_TYPE_PRESENT) || (pAce->Flags == ACE_INHERITED_OBJECT_TYPE_PRESENT))
                    {
                        // If Flags contains only one flag (either ACE_OBJECT_TYPE_PRESENT
                        // or ACE_INHERITED_OBJECT_TYPE_PRESENT), the SidStart member starts at
                        // the offset specified by the InheritedObjectType member.
                        pSidToCheck = &pAceToCheck->InheritedObjectType;
                        pSidAce = &pAce->InheritedObjectType;

                        if ((EqualSid(pSidToCheck, pSidAce)) &&
                            (GUID_EQ(&pAceToCheck->ObjectType, &pAce->ObjectType)))
                            return TRUE;
                    }
                    else
                    {
                        pSidToCheck = &pAceToCheck->SidStart;
                        pSidAce = &pAce->SidStart;

                        if ((EqualSid(pSidToCheck, pSidAce)) &&
                            (GUID_EQ(&pAceToCheck->ObjectType, &pAce->ObjectType)) &&
                            (GUID_EQ(&pAceToCheck->InheritedObjectType, &pAce->InheritedObjectType)))
                            return TRUE;
                    }
                }
            }
            else
            {
                FATAL_FCT(_T("Unknown ACE type <%u>"), pAceHeader->AceType);
            }
        }
    }

    return FALSE;
}

// Warning : caller must free the returned value with LocalFree
DWORD * ParseObjectClasses(
    _Inout_  LPTSTR   szObjectClasses,
    _Out_    DWORD    *pdwObjectClassCount
    ) {
    DWORD *pdwObjectClassArray = NULL;
    DWORD idx = 0;
    LPTSTR token = NULL;
    LPTSTR ctx = NULL;

    *pdwObjectClassCount = 1;

    for (DWORD i = 0; szObjectClasses[i] != _T('\0'); i++) {
        if (szObjectClasses[i] == ',') {
            (*pdwObjectClassCount)++;
        }
    }

    pdwObjectClassArray = (DWORD *)LocalAllocCheckX(sizeof(DWORD)* (*pdwObjectClassCount));
    while (StrNextToken(szObjectClasses, ",", &ctx, &token)) {
        pdwObjectClassArray[idx++] = _tcstoul(token, NULL, 16);
    }

    return pdwObjectClassArray;
}

HRESULT ConvertStrGuidToGuid(
    _In_ LPTSTR strGuid,
    _Out_ GUID * guid
    ) {
    WCHAR formated[GUID_STR_SIZE_NULL] = { 0 };
    size_t size = 0;

    size = swprintf(formated, _countof(formated), strGuid[0] != '{' ? L"{%S}" : L"%S", strGuid);
    if (size == -1) {
        FATAL(_T("Cannot format GUID <%s> for conversion"), strGuid);
    }

    LOG(All, _T("Attempting to convert GUID %ws to binary form"), formated);

    return CLSIDFromString((LPCOLESTR)formated, guid);
}

// From : http://msdn.microsoft.com/en-us/library/windows/desktop/aa446619(v=vs.85).aspx
BOOL SetPrivilege(
    _Inout_ HANDLE hToken,
    _In_ LPCTSTR lpszPrivilege,
    _In_ BOOL bEnablePrivilege
    ) {
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup 
        &luid))        // receives LUID of privilege
    {
        LOG(Err, "LookupPrivilegeValue error : <%u>", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        LOG(Err, "AdjustTokenPrivileges error : <%u>", GetLastError());
        return FALSE;
    }

    return (GetLastError() == ERROR_SUCCESS);
}

BOOL EnablePrivForCurrentProcess(
    _In_ LPTSTR szPrivilege
    ) {
    BOOL bResult = FALSE;
    HANDLE hToken = 0;

    bResult = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!bResult) {
        LOG(Err, "Cannot open process token : <%u>", GetLastError());
        return FALSE;
    }

    bResult = SetPrivilege(hToken, szPrivilege, TRUE);
    if (!bResult && GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        LOG(Err, "Current process does not have the privilege <%s>", szPrivilege);
        return FALSE;
    }

    return TRUE;
}
