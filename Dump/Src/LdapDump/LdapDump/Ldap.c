// Code mainly taken from another project by L.Delsalle and adapted/completed for this project.

/* --- INCLUDES ------------------------------------------------------------- */
#include "Ldap.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
HANDLE hLdapHeap = INVALID_HANDLE_VALUE;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
BOOL LdapInit(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    ) {
    DWORD dwRes = LDAP_SUCCESS;
    ULONG version = LDAP_VERSION3;
    ULONG size = LDAP_NO_LIMIT;
    ULONG time = LDAP_NO_LIMIT;

    hLdapHeap = HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
    if (hLdapHeap == NULL) {
        FATAL(_T("Error during heap creation"));
    }

    if (!pLdapOptions->tLDAPServer || (pLdapOptions->dwLDAPPort == 0)) {
        LOG(Err, _T("Invalid LDAP server/port"));
        return FALSE;
    }

    pLDAPConnectInfo->hLDAPConnection = NULL;
    pLDAPConnectInfo->ptLDAPDomainDN = NULL;

    // Connection initialization
    pLDAPConnectInfo->hLDAPConnection = ldap_init(pLdapOptions->tLDAPServer, pLdapOptions->dwLDAPPort);
    if (!pLDAPConnectInfo->hLDAPConnection) {
        LOG(Err, _T("Unable to init LDAP connection for %s:%u (ErrorCode: 0x%x)"), pLdapOptions->tLDAPServer, pLdapOptions->dwLDAPPort, LdapGetLastError());
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    // Define connection options
    dwRes = ldap_set_option(pLDAPConnectInfo->hLDAPConnection, LDAP_OPT_PROTOCOL_VERSION, (void*)&version);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to set LDAP protocole option (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }
    dwRes = ldap_set_option(pLDAPConnectInfo->hLDAPConnection, LDAP_OPT_SIZELIMIT, (void*)&size);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to set LDAP size option (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }
    dwRes = ldap_set_option(pLDAPConnectInfo->hLDAPConnection, LDAP_OPT_TIMELIMIT, (void*)&time);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to set LDAP size option (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    // Connect to server
    dwRes = ldap_connect(pLDAPConnectInfo->hLDAPConnection, NULL);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to connect to LDAP server (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    return TRUE;
}

BOOL LdapBind(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PTCHAR ptNamingContext,
    _In_ PTCHAR ptUserName,
    _In_ PTCHAR ptPassword,
    _In_opt_ PTCHAR ptExplicitDomain
    ) {
    DWORD dwRes = LDAP_SUCCESS;
    SEC_WINNT_AUTH_IDENTITY sSecIdent;

    if (!pLDAPConnectInfo || !pLDAPConnectInfo->hLDAPConnection || !ptNamingContext) {
        LOG(Err, _T("Invalid LDAP connection"));
        return FALSE;
    }

    // Init bind data
    if (ptUserName == NULL || ptPassword == NULL) {
        sSecIdent.User = NULL;
        sSecIdent.UserLength = 0;
        sSecIdent.Password = NULL;
        sSecIdent.PasswordLength = 0;
        sSecIdent.Domain = NULL;
        sSecIdent.DomainLength = 0;
        sSecIdent.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }
    else {
        sSecIdent.User = (unsigned char*)ptUserName;
        sSecIdent.UserLength = (unsigned long)_tcslen(ptUserName);
        sSecIdent.Password = (unsigned char*)ptPassword;
        sSecIdent.PasswordLength = (unsigned long)_tcslen(ptPassword);
        sSecIdent.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

        if (ptExplicitDomain == NULL) {
            sSecIdent.Domain = (unsigned char*)pLDAPConnectInfo->ptLDAPDomainName;
            sSecIdent.DomainLength = (unsigned long)_tcslen(pLDAPConnectInfo->ptLDAPDomainName);
        }
        else {
            sSecIdent.Domain = (unsigned char*)ptExplicitDomain;
            sSecIdent.DomainLength = (unsigned long)_tcslen(ptExplicitDomain);
        }
    }

    LOG(Dbg, _T("Ldap binding data <domain:%s> <username:%s>"), sSecIdent.Domain, sSecIdent.User);


    // Server bind
    dwRes = ldap_bind_s(pLDAPConnectInfo->hLDAPConnection, ptNamingContext, (PTCHAR)&sSecIdent, LDAP_AUTH_NEGOTIATE);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to bind to LDAP server (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    return TRUE;
}

BOOL LdapDisconnectAndDestroy(
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    ) {
    if (!pLDAPConnectInfo) {
        FATAL(_T("LDAP_CONNECT_INFO pointer invalid"));
    }

    if (pLDAPConnectInfo->hLDAPConnection)
        ldap_unbind_s(pLDAPConnectInfo->hLDAPConnection);
    pLDAPConnectInfo->hLDAPConnection = NULL;

    if (pLDAPConnectInfo->ptLDAPDomainDN)
        HeapFree(hLdapHeap, 0, pLDAPConnectInfo->ptLDAPDomainDN);
    if (pLDAPConnectInfo->ptLDAPDomainName)
        HeapFree(hLdapHeap, 0, pLDAPConnectInfo->ptLDAPDomainName);
    if (pLDAPConnectInfo->ptLDAPConfigDN)
        HeapFree(hLdapHeap, 0, pLDAPConnectInfo->ptLDAPConfigDN);
    if (pLDAPConnectInfo->ptLDAPSchemaDN)
        HeapFree(hLdapHeap, 0, pLDAPConnectInfo->ptLDAPSchemaDN);

    HeapDestroy(hLdapHeap);

    return TRUE;
}

BOOL LdapPagedSearch(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PTCHAR tLdapBaseDn,
    _In_ PTCHAR tLdapFilter,
    _In_ PTCHAR tOrigAttribute,
    _In_ PLDAPControl *pLdapServerControls,
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    ) {
    DWORD dwRes = LDAP_SUCCESS, dwCountEntries = 0;
    PLDAPMessage pLDAPSearchResult = NULL;
    PLDAPMessage pCurrentEntry = NULL;
    PTCHAR ptCurrAttribute = NULL;
    BerElement* pBerElt = NULL;
    PLDAPSearch pLDAPSearch = NULL;
    struct l_timeval timeout;
    DWORD dwAllValuesCount = 0;
    PTCHAR pAttributes[2] = { NULL };
    PLDAP_RETRIEVED_DATA *pRetrievedAllData = NULL;
    pAttributes[0] = tOrigAttribute;

    if (!tLdapFilter || !tOrigAttribute) {
        LOG(Err, _T("Invalid LDAP filter or attribute"));
        return FALSE;
    }

    pLDAPSearch = ldap_search_init_page(pLDAPConnectInfo->hLDAPConnection, tLdapBaseDn,
        LDAP_SCOPE_SUBTREE, tLdapFilter, pAttributes, FALSE, pLdapServerControls, NULL, LDAP_NO_LIMIT, LDAP_NO_LIMIT, NULL);
    if (pLDAPSearch == NULL) {
        LOG(Err, _T("Unable to create ldap search structure (ErrorCode: 0x%0lX)"), LdapGetLastError());
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    // We loop until all the LDAP pages have been returned
    for (;;) {
        DWORD dwPageEntries = 0;
        PLDAP_RETRIEVED_DATA *pRetrievedPageEntriesData = NULL;
        BOOL bIsAttributeFound = FALSE;

        timeout.tv_sec = 100000;
        dwRes = ldap_get_next_page_s(pLDAPConnectInfo->hLDAPConnection, pLDAPSearch, &timeout, AD_LDAP_SEARCH_LIMIT, &dwCountEntries, &pLDAPSearchResult);
        if (dwRes != LDAP_SUCCESS) {
            if (dwRes == LDAP_NO_RESULTS_RETURNED)
                break;

            LOG(Err, _T("Unable to retrieve paged count (ErrorCode: 0x%0lX)"), dwRes);
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
            return FALSE;
        }

        dwPageEntries = ldap_count_entries(pLDAPConnectInfo->hLDAPConnection, pLDAPSearchResult);
        if (!dwPageEntries)
            continue;

        pRetrievedPageEntriesData = (PLDAP_RETRIEVED_DATA *)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, sizeof (PLDAP_RETRIEVED_DATA)* (dwPageEntries));
        if (!pRetrievedPageEntriesData) {
            LOG(Err, _T("Unable to allocate PLDAP_RETRIEVED_DATA* memory"));
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
            return FALSE;
        }
        ZeroMemory(pRetrievedPageEntriesData, sizeof (PLDAP_RETRIEVED_DATA)* (dwPageEntries));

        pCurrentEntry = ldap_first_entry(pLDAPConnectInfo->hLDAPConnection, pLDAPSearchResult);
        if (!pCurrentEntry) {
            LOG(Err, _T("Unable to retrieve pCurrentEntry"));
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
            return FALSE;
        }

        // We loop until all returned objects have been processed
        for (DWORD i = 0; i < dwPageEntries; ++i) {
            PLDAP_RETRIEVED_DATA pRetrievedData = NULL;

            ptCurrAttribute = ldap_first_attribute(pLDAPConnectInfo->hLDAPConnection, pCurrentEntry, &pBerElt);
            ldap_next_attribute(pLDAPConnectInfo->hLDAPConnection, pCurrentEntry, pBerElt);
            if (!ptCurrAttribute)
                continue;
            else
                bIsAttributeFound = TRUE;

            // Select the right method to retrieve attributes (directly or by range)
            if (!_tcsicmp(tOrigAttribute, ptCurrAttribute)) {
                if (LdapExtractAttributes(pLDAPConnectInfo, pCurrentEntry, ptCurrAttribute, &pRetrievedData) != TRUE) {
                    LOG(Err, _T("Unable to extract LDAP attributes"));
                    ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
                    return FALSE;
                }
            }
            else {
                if (LdapExtractRangedAttributes(pLDAPConnectInfo, pCurrentEntry, tOrigAttribute, ptCurrAttribute, &pRetrievedData) != TRUE) {
                    LOG(Err, _T("Unable to extract LDAP rangedattributes"));
                    ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
                    return FALSE;
                }
            }

            if (ptCurrAttribute) {
                ldap_memfree(ptCurrAttribute);
                ptCurrAttribute = NULL;
            }
            pRetrievedPageEntriesData[i] = pRetrievedData;
        }

        if (bIsAttributeFound) {
            pRetrievedAllData = (dwAllValuesCount) ? (PLDAP_RETRIEVED_DATA *)HeapReAlloc(hLdapHeap, HEAP_ZERO_MEMORY, pRetrievedAllData, sizeof(PLDAP_RETRIEVED_DATA)* (dwAllValuesCount + dwPageEntries))
                : (PLDAP_RETRIEVED_DATA *)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(PLDAP_RETRIEVED_DATA)* (dwPageEntries));

            if (!pRetrievedAllData) {
                return FALSE;
            }

            for (DWORD k = 0; k < dwPageEntries; ++k) {
                pRetrievedAllData[dwAllValuesCount + k] = pRetrievedPageEntriesData[k];
            }
            dwAllValuesCount += dwPageEntries;
        }
        if (pRetrievedPageEntriesData)
            HeapFree(hLdapHeap, 0, pRetrievedPageEntriesData);
    }

    *ppRetrievedResults = pRetrievedAllData;
    *dwResultsCount = dwAllValuesCount;
    return TRUE;
}

BOOL LdapFreePagedSearchResults(
    _Inout_ PLDAP_RETRIEVED_DATA **ppRetrievedResults,
    _Inout_ PDWORD dwResultsCount
    ) {
    DWORD i = 0, j = 0;

    for (i = 0; i < *dwResultsCount; i++) {
        for (j = 0; j < (*ppRetrievedResults)[i]->dwElementCount; j++) {
            HeapFree(hLdapHeap, 0, (*ppRetrievedResults)[i]->ppbData[j]);
            (*ppRetrievedResults)[i]->ppbData[j] = NULL;
        }

        HeapFree(hLdapHeap, 0, (*ppRetrievedResults)[i]->ppbData);
        (*ppRetrievedResults)[i]->ppbData = NULL;

        HeapFree(hLdapHeap, 0, (*ppRetrievedResults)[i]->pdwDataSize);
        (*ppRetrievedResults)[i]->pdwDataSize = NULL;

        HeapFree(hLdapHeap, 0, (*ppRetrievedResults)[i]);
        (*ppRetrievedResults)[i] = NULL;
    }

    HeapFree(hLdapHeap, 0, *ppRetrievedResults);
    *ppRetrievedResults = NULL;
    *dwResultsCount = 0;

    return TRUE;
}

BOOL LdapExtractAttributes(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    ) {
    DWORD dwAttributeCount = 0;
    PTCHAR tCurrentDN = ldap_get_dn(pLDAPConnectInfo->hLDAPConnection, pCurrentEntry);
    PBYTE *ppbData = NULL;
    PDWORD pdwDataSize = NULL;
    struct berval **bvals = NULL;

    if (!pCurrentEntry || !tAttribute) {
        LOG(Err, _T("Unable to extract attributes"));
        return FALSE;
    }

    if (!tCurrentDN) {
        LOG(Err, _T("Unable to retrieve DN for current entry"));
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    bvals = ldap_get_values_len(pLDAPConnectInfo->hLDAPConnection, pCurrentEntry, tAttribute);
    if (!bvals) {
        LOG(Warn, _T("Unable to retrieve bvals attributes for DN: %s"), tCurrentDN);
        goto FreeAndContinue;
    }

    for (DWORD j = 0; bvals[j] != NULL; ++j) {
        PBYTE pbTmpData = NULL;

        ppbData = ppbData ? (PBYTE *)HeapReAlloc(hLdapHeap, 0, ppbData, sizeof(PBYTE)* (j + 1))
            : (PBYTE *)HeapAlloc(hLdapHeap, 0, sizeof(PBYTE));
        pdwDataSize = pdwDataSize ? (PDWORD)HeapReAlloc(hLdapHeap, 0, pdwDataSize, sizeof(DWORD)* (j + 1))
            : (PDWORD)HeapAlloc(hLdapHeap, 0, sizeof(DWORD));
        pbTmpData = (PBYTE)HeapAlloc(hLdapHeap, 0, sizeof(BYTE)* (bvals[j]->bv_len + 1));
        if (!ppbData || !pdwDataSize || !pbTmpData) {
            LOG(Err, _T("Unable to allocate memory for extracted attribute"));
            return FALSE;
        }
        ZeroMemory(pbTmpData, sizeof(BYTE)* (bvals[j]->bv_len + 1));

        if (memcpy_s(pbTmpData, sizeof(BYTE)* (bvals[j]->bv_len + 1), bvals[j]->bv_val, sizeof(BYTE)* (bvals[j]->bv_len))) {
            LOG(Warn, _T("Unable to copy attribute data"));
            return FALSE;
        }

        ppbData[j] = pbTmpData;
        pdwDataSize[j] = bvals[j]->bv_len;
        dwAttributeCount++;
    }

FreeAndContinue:
    if (bvals != NULL)
    {
        ldap_value_free_len(bvals);
        bvals = NULL;
    }

    *ppRetrievedData = (PLDAP_RETRIEVED_DATA)HeapAlloc(hLdapHeap, 0, sizeof(LDAP_RETRIEVED_DATA));
    if (!*ppRetrievedData) {
        LOG(Err, _T("Unable to allocate LDAP_RETRIEVED_DATA"));
        return FALSE;
    }
    ZeroMemory(*ppRetrievedData, sizeof(LDAP_RETRIEVED_DATA));
    (*ppRetrievedData)->dwElementCount = dwAttributeCount;
    (*ppRetrievedData)->ppbData = ppbData;
    (*ppRetrievedData)->pdwDataSize = pdwDataSize;
    (*ppRetrievedData)->tDN = tCurrentDN;

    return TRUE;
}

BOOL LdapExtractRangedAttributes(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _In_ PLDAPMessage pCurrentEntry,
    _In_ PTCHAR tOrigAttribute,
    _In_ PTCHAR tAttribute,
    _Inout_ PLDAP_RETRIEVED_DATA *ppRetrievedData
    ) {
    LDAPMessage *pLDAPSearchResult, *pLDAPEntry;
    PTCHAR pNewRangeAttributes[2] = { NULL };
    PTCHAR tTmpNewAttribute = NULL;
    DWORD dwStart = 0, dwEnd = 0;
    PTCHAR tCurrentDN = ldap_get_dn(pLDAPConnectInfo->hLDAPConnection, pCurrentEntry);
    DWORD dwAttributeCount = 0;
    PBYTE *ppbData = NULL;
    PDWORD pdwDataSize = NULL;
    pNewRangeAttributes[0] = tAttribute;

    if (!pCurrentEntry || !tAttribute || !tCurrentDN) {
        LOG(Err, _T("Invalid LDAP ranged attribute request"));
        return FALSE;
    }

    do {
        DWORD dwCurrAttributeCount = 0;
        PBYTE *ppbCurrData = NULL;
        PDWORD pdwCurrDataSize = NULL;

        if (ldap_search_s(pLDAPConnectInfo->hLDAPConnection, tCurrentDN, LDAP_SCOPE_BASE, TEXT("(objectClass=*)"), pNewRangeAttributes, FALSE, &pLDAPSearchResult) != LDAP_SUCCESS) {
            FATAL(_T("Unable to search for range attribute (err=%d)"), LdapGetLastError());
        }

        pLDAPEntry = ldap_first_entry(pLDAPConnectInfo->hLDAPConnection, pLDAPSearchResult);
        if (!pLDAPEntry) {
            FATAL(_T("Unable to retrieve entry for range attributes (err=%d)"), LdapGetLastError());
        }

        if (GetRangeValues(pLDAPConnectInfo, pLDAPEntry, tAttribute, &dwCurrAttributeCount, &ppbCurrData, &pdwCurrDataSize) == FALSE) {
            FATAL(_T("Unable to extract range values for range attributes"));
        }

        ppbData = ppbData ? (PBYTE *)HeapReAlloc(hLdapHeap, 0, ppbData, sizeof(PBYTE)* (dwAttributeCount + dwCurrAttributeCount))
            : (PBYTE *)HeapAlloc(hLdapHeap, 0, sizeof(PBYTE)* dwCurrAttributeCount);
        pdwDataSize = pdwDataSize ? (PDWORD)HeapReAlloc(hLdapHeap, 0, pdwDataSize, sizeof(DWORD)* (dwAttributeCount + dwCurrAttributeCount))
            : (PDWORD)HeapAlloc(hLdapHeap, 0, sizeof(DWORD)* dwCurrAttributeCount);

        if (!ppbData || !pdwDataSize) {
            FATAL(_T("Memory exhaustion. Unable to allocate ppbData and ppbDataSize"));
        }

        for (DWORD j = 0; j < dwCurrAttributeCount; ++j) {
            ppbData[dwAttributeCount + j] = ppbCurrData[j];
            pdwDataSize[dwAttributeCount + j] = pdwCurrDataSize[j];
        }
        dwAttributeCount += dwCurrAttributeCount;

        if (ppbCurrData)
            HeapFree(hLdapHeap, 0, ppbCurrData);
        if (pdwCurrDataSize)
            HeapFree(hLdapHeap, 0, pdwCurrDataSize);

        if (ParseRange(tOrigAttribute, pNewRangeAttributes[0], &dwStart, &dwEnd) == FALSE) {
            FATAL(_T("Unable to parse current range"));
        }

        if (tTmpNewAttribute)
            HeapFree(hLdapHeap, 0, tTmpNewAttribute);
        if (ConstructRangeAtt(tOrigAttribute, dwEnd + 1, -1, &tTmpNewAttribute) == FALSE) {
            FATAL(_T("Unable to construct new range"));
        }
        pNewRangeAttributes[0] = tTmpNewAttribute;

    } while (dwEnd != -1);
    if (tTmpNewAttribute)
        HeapFree(hLdapHeap, 0, tTmpNewAttribute);

    *ppRetrievedData = (PLDAP_RETRIEVED_DATA)HeapAlloc(hLdapHeap, 0, sizeof(LDAP_RETRIEVED_DATA));
    if (!*ppRetrievedData) {
        LOG(Err, _T("Unable to allocate LDAP_RETRIEVED_DATA"));
        return FALSE;
    }
    ZeroMemory(*ppRetrievedData, sizeof(LDAP_RETRIEVED_DATA));
    (*ppRetrievedData)->dwElementCount = dwAttributeCount;
    (*ppRetrievedData)->ppbData = ppbData;
    (*ppRetrievedData)->pdwDataSize = pdwDataSize;
    (*ppRetrievedData)->tDN = tCurrentDN;
    return TRUE;
}

BOOL GetRangeValues(
    _In_ PLDAP_CONNECT_INFO pLDAPConnectInfo,
    _Inout_ PLDAPMessage pEntry,
    _In_ PTCHAR tOriginalAttribute,
    _Inout_ PDWORD pdwAttributeCount,
    _Inout_ PBYTE **pppbData,
    _Inout_ PDWORD *ppdwDataSize
    ) {
    PTCHAR ptAttribute = NULL;
    BerElement *pBerElt = NULL;
    DWORD dwAttributeCount = 0;
    PBYTE *ppbData = NULL;
    PDWORD pdwDataSize = NULL;
    struct berval **bvals = NULL;

    UNREFERENCED_PARAMETER(tOriginalAttribute);

    ptAttribute = ldap_first_attribute(pLDAPConnectInfo->hLDAPConnection, pEntry, &pBerElt);
    if (!ptAttribute) {
        FATAL(_T("Unable to extract ranged attribute (err=%d)"), LdapGetLastError());
    }

    bvals = ldap_get_values_len(pLDAPConnectInfo->hLDAPConnection, pEntry, ptAttribute);
    if (!bvals) {
        LOG(Warn, _T("Unable to retrieve bvals attributes"));
        goto FreeAndContinue;
    }

    for (DWORD j = 0; bvals[j] != NULL; ++j) {
        PBYTE pbTmpData = NULL;

        ppbData = ppbData ? (PBYTE *)HeapReAlloc(hLdapHeap, 0, ppbData, sizeof(PBYTE)* (j + 1))
            : (PBYTE *)HeapAlloc(hLdapHeap, 0, sizeof(PBYTE));
        pdwDataSize = pdwDataSize ? (PDWORD)HeapReAlloc(hLdapHeap, 0, pdwDataSize, sizeof(DWORD)* (j + 1))
            : (PDWORD)HeapAlloc(hLdapHeap, 0, sizeof(DWORD));
        pbTmpData = (PBYTE)HeapAlloc(hLdapHeap, 0, sizeof(BYTE)* (bvals[j]->bv_len + 1));
        if (!ppbData || !pdwDataSize || !pbTmpData) {
            LOG(Err, _T("Unable to allocate memory for extracted attribute"));
            return FALSE;
        }
        ZeroMemory(pbTmpData, sizeof(BYTE)* (bvals[j]->bv_len + 1));

        if (memcpy_s(pbTmpData, sizeof(BYTE)* (bvals[j]->bv_len + 1), bvals[j]->bv_val, sizeof(BYTE)* (bvals[j]->bv_len))) {
            LOG(Warn, _T("Unable to copy attribute data"));
            return FALSE;
        }

        ppbData[j] = pbTmpData;
        pdwDataSize[j] = bvals[j]->bv_len;
        dwAttributeCount++;
    }

FreeAndContinue:
    if (bvals != NULL) {
        ldap_value_free_len(bvals);
        bvals = NULL;
    }
    if (ptAttribute) {
        ldap_memfree(ptAttribute);
        ptAttribute = NULL;
    }
    if (pBerElt)
        ber_free(pBerElt, 0);

    *pdwAttributeCount = dwAttributeCount;
    *pppbData = ppbData;
    *ppdwDataSize = pdwDataSize;
    return TRUE;
}

BOOL ParseRange(
    _In_ PTCHAR tAtttype,
    _In_ PTCHAR tAttdescr,
    _Inout_ PDWORD pdwStart,
    _Inout_ PDWORD pdwEnd
    ) {
    PTCHAR tRangeStr = TEXT("range=");
    PTCHAR tStartstring = NULL, tEndstring = NULL, tOptionstart = NULL;
    INT iAtttypeLen = 0, iAttdescrLen = 0;
    DWORD dwRangestringlen = 0, dwCpt = 0;
    BOOL bRangefound = FALSE;

    dwRangestringlen = (DWORD)_tcslen(tRangeStr);

    if (!_tcsicmp(tAtttype, tAttdescr)) {
        // The attribute was returned without options.
        *pdwStart = 0;
        *pdwEnd = (DWORD)-1;
        return TRUE;
    }

    iAtttypeLen = (INT)_tcslen(tAtttype);
    iAttdescrLen = (INT)_tcslen(tAttdescr);

    if ((iAtttypeLen > iAttdescrLen) || (';' != tAttdescr[iAtttypeLen]) || (_tcsnicmp(tAtttype, tAttdescr, iAtttypeLen)))
        return FALSE;

    // It is the correct attribute type. Verify if there is a range option.
    *pdwStart = 0;
    *pdwEnd = (DWORD)-1;
    tOptionstart = tAttdescr + iAtttypeLen + 1;
    do {
        if ((iAttdescrLen - (tOptionstart - tAttdescr)) < (INT)dwRangestringlen) {
            // No space left in the string for range option.
            tOptionstart = tAttdescr + iAttdescrLen;
        }
        else if (!_tcsncicmp(tOptionstart, tRangeStr, dwRangestringlen)) {
            // Found a range string. Ensure that it looks like what is expected and then parse it.
            tStartstring = tOptionstart + dwRangestringlen;
            for (dwCpt = 0; isdigit((UCHAR)tStartstring[dwCpt]); dwCpt++);

            if ((0 != dwCpt) && ('-' == tStartstring[dwCpt]) && ((tStartstring[dwCpt + 1] == '*') || isdigit((UCHAR)tStartstring[dwCpt + 1]))) {
                // Acceptable. Finish parsing.
                tEndstring = &tStartstring[dwCpt + 1];
                *pdwStart = _tcstol(tStartstring, NULL, 10);
                if (tEndstring[0] == '*')
                    *pdwEnd = (DWORD)-1;
                else
                    *pdwEnd = _tcstol(tEndstring, NULL, 10);
                bRangefound = TRUE;
            }
        }

        // If necessary, advance to the next option.
        if (!bRangefound) {
            while ((*tOptionstart != '\0') && (*tOptionstart != ';'))
                tOptionstart++;

            // Skip the semicolon.
            tOptionstart++;
        }
    } while (!bRangefound && (iAttdescrLen > (tOptionstart - tAttdescr)));

    return TRUE;
}

BOOL ConstructRangeAtt(
    _In_ PTCHAR tAtttype,
    _In_ DWORD dwStart,
    _In_ INT iEnd,
    _Out_ PTCHAR* tOutputRangeAttr
    ) {
    TCHAR startstring[11], endstring[11];
    DWORD requiredlen = 0;
    PTCHAR tOutbuff = NULL;

    startstring[10] = TEXT('\0');
    endstring[10] = TEXT('\0');

    // Calculate buffer space required.
    _stprintf_s(startstring, _countof(startstring), TEXT("%u"), dwStart);
    if (iEnd == -1)
        _tcscpy_s(endstring, _countof(endstring), TEXT("*"));
    else
        _stprintf_s(endstring, _countof(endstring), TEXT("%u"), (ULONG)iEnd);

    // Add in space for ';range=' and '-' and the terminating null.
    requiredlen = (DWORD)(_tcslen(tAtttype) + _tcslen(startstring) + _tcslen(endstring));
    requiredlen += 9;

    // Verify that enough space was passed in.
    tOutbuff = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (requiredlen + 1) * sizeof(TCHAR));
    if (!tOutbuff) {
        FATAL(_T("Unable to allocate raneg attribute"));
    }
    _sntprintf_s(tOutbuff, (requiredlen + 1), requiredlen, TEXT("%s;range=%s-%s"), tAtttype, startstring, endstring);
    tOutbuff[requiredlen] = '\0';

    *tOutputRangeAttr = tOutbuff;
    return TRUE;
}

BOOL ExtractDomainNamingContexts(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _Inout_ PLDAP_CONNECT_INFO pLDAPConnectInfo
    ) {
    PLDAPMessage plmsgSearchResponse = NULL;
    LDAPMessage* pFirstEntry = NULL;
    DWORD dwRes = LDAP_SUCCESS;
    PTCHAR *pNamingContextTmp = NULL, *pLdapServiceName = NULL;
    DWORD dwStrSize = 0, dnsDomainLen = 0;
    BOOL bIsMissingDnsName = FALSE;

    if (!pLDAPConnectInfo || !pLDAPConnectInfo->hLDAPConnection) {
        LOG(Err, _T("LDAP connection or pNamingContext invalid"));
        return FALSE;
    }

    // Annonym bind
    dwRes = ldap_bind_s(pLDAPConnectInfo->hLDAPConnection, NULL, NULL, LDAP_AUTH_SIMPLE);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to bind anonymously to LDAP server in order to extract naming context (ErrorCode: 0x%0lX)"), dwRes);
        if (pLDAPConnectInfo->hLDAPConnection)
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        return FALSE;
    }

    // C6387	Invalid parameter value	'_Param_(4)' could be '0':  this does not adhere to the specification for the function 'ldap_search_s'
    // Fake : the notation is wrong.
#pragma warning(suppress: 6387)
    // Determine data NC from rootDSE
    dwRes = ldap_search_s(pLDAPConnectInfo->hLDAPConnection, NULL, LDAP_SCOPE_BASE, NULL, NULL, 0, &plmsgSearchResponse);
    if (dwRes != LDAP_SUCCESS) {
        LOG(Err, _T("Unable to search root dse (ErrorCode: 0x%0lX)"), dwRes);
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        if (plmsgSearchResponse != NULL)
            ldap_msgfree(plmsgSearchResponse);
        return FALSE;
    }

    pFirstEntry = ldap_first_entry(pLDAPConnectInfo->hLDAPConnection, plmsgSearchResponse);
    if (!pFirstEntry) {
        LOG(Err, _T("Unable to extract domain name from root dse"));
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        if (plmsgSearchResponse != NULL)
            ldap_msgfree(plmsgSearchResponse);
        return FALSE;
    }

    pNamingContextTmp = ldap_get_values(pLDAPConnectInfo->hLDAPConnection, pFirstEntry, TEXT("defaultNamingContext"));
    if (!pNamingContextTmp) {
        LOG(Err, _T("Unable to extract domain name from root dse, pNamingContext invalid"));
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        if (plmsgSearchResponse != NULL)
            ldap_msgfree(plmsgSearchResponse);
        return FALSE;
    }

    dwStrSize = (DWORD)_tcslen(*pNamingContextTmp);
    pLDAPConnectInfo->ptLDAPDomainDN = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (dwStrSize + 1) * sizeof(TCHAR));
    if (!pLDAPConnectInfo->ptLDAPDomainDN) {
        LOG(Warn, _T("Unable to allocate ptLDAPDomainDN structure"));
        return FALSE;
    }

    if (_tcscpy_s(pLDAPConnectInfo->ptLDAPDomainDN, dwStrSize + 1, *pNamingContextTmp)) {
        LOG(Warn, _T("Unable to copy pNamingContext structure to ptLDAPDomainDN"));
        return FALSE;
    }

    // Determine config NC from rootDSE
    pNamingContextTmp = ldap_get_values(pLDAPConnectInfo->hLDAPConnection, pFirstEntry, TEXT("configurationNamingContext"));
    if (!pNamingContextTmp) {
        LOG(Err, _T("Unable to extract config NC from root dse, pNamingContext invalid"));
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        if (plmsgSearchResponse != NULL)
            ldap_msgfree(plmsgSearchResponse);
        return FALSE;
    }

    dwStrSize = (DWORD)_tcslen(*pNamingContextTmp);
    pLDAPConnectInfo->ptLDAPConfigDN = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (dwStrSize + 1) * sizeof(TCHAR));
    if (!pLDAPConnectInfo->ptLDAPConfigDN) {
        LOG(Warn, _T("Unable to allocate ptLDAPConfigDN structure"));
        return FALSE;
    }

    if (_tcscpy_s(pLDAPConnectInfo->ptLDAPConfigDN, dwStrSize + 1, *pNamingContextTmp)) {
        LOG(Warn, _T("Unable to copy pNamingContext structure to ptLDAPConfigDN"));
        return FALSE;
    }

    // Determine schema NC from rootDSE
    pNamingContextTmp = ldap_get_values(pLDAPConnectInfo->hLDAPConnection, pFirstEntry, TEXT("schemaNamingContext"));
    if (!pNamingContextTmp) {
        LOG(Err, _T("Unable to extract schema NC from root dse, pNamingContext invalid"));
        ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
        if (plmsgSearchResponse != NULL)
            ldap_msgfree(plmsgSearchResponse);
        return FALSE;
    }

    dwStrSize = (DWORD)_tcslen(*pNamingContextTmp);
    pLDAPConnectInfo->ptLDAPSchemaDN = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (dwStrSize + 1) * sizeof(TCHAR));
    if (!pLDAPConnectInfo->ptLDAPSchemaDN) {
        LOG(Warn, _T("Unable to allocate ptLDAPSchemaDN structure"));
        return FALSE;
    }

    if (_tcscpy_s(pLDAPConnectInfo->ptLDAPSchemaDN, dwStrSize + 1, *pNamingContextTmp)) {
        LOG(Warn, _T("Unable to copy pNamingContext structure to ptLDAPSchemaDN"));
        return FALSE;
    }

    // Determine domain DNS name from rootDSE
    pLdapServiceName = ldap_get_values(pLDAPConnectInfo->hLDAPConnection, pFirstEntry, TEXT("ldapServiceName"));
    if (!pLdapServiceName) {
        // If the domain DNS name is missing (eg: when mounting ntds.dit export)
        if (pLdapOptions->tDNSName) {
            PTCHAR pDomainDnsName = NULL;
            DWORD dwDnsDomainNameLen = (DWORD)_tcslen(pLdapOptions->tDNSName);

            bIsMissingDnsName = TRUE;
            pDomainDnsName = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (dwDnsDomainNameLen + 1) *sizeof(TCHAR));
            if (!pDomainDnsName) {
                LOG(Warn, _T("Unable to allocate pLdapServiceName structure"));
                return FALSE;
            }

            if (memcpy_s(pDomainDnsName, (dwDnsDomainNameLen + 1) * sizeof(TCHAR), pLdapOptions->tDNSName, dwDnsDomainNameLen * sizeof(TCHAR))) {
                LOG(Warn, _T("Unable to copy ptLDAPDomainName structure"));
                return FALSE;
            }
            pDomainDnsName[dwDnsDomainNameLen] = '\0';
            pLdapServiceName = &pDomainDnsName;
        }
        else {
            LOG(Err, _T("Unable to extract dns domain name from root dse, pLdapServiceName invalid"));
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
            if (plmsgSearchResponse != NULL)
                ldap_msgfree(plmsgSearchResponse);
            return FALSE;
        }
    }

    if (bIsMissingDnsName == FALSE) {
        dnsDomainLen = ((DWORD)(_tcschr(*pLdapServiceName, TEXT(':'))) - (DWORD)(*pLdapServiceName)) / (DWORD) sizeof(TCHAR);
        if (dnsDomainLen == 0) {
            LOG(Err, _T("Unable to extract dns domain name from root dse, dnsDomainLen invalid"));
            ldap_unbind(pLDAPConnectInfo->hLDAPConnection);
            if (plmsgSearchResponse != NULL)
                ldap_msgfree(plmsgSearchResponse);
            return FALSE;
        }
    }
    else {
        dnsDomainLen = (DWORD)_tcslen(*pLdapServiceName);
    }

    pLDAPConnectInfo->ptLDAPDomainName = (PTCHAR)HeapAlloc(hLdapHeap, HEAP_ZERO_MEMORY, (dnsDomainLen + 1) * sizeof(TCHAR));
    if (!pLDAPConnectInfo->ptLDAPDomainName) {
        LOG(Warn, _T("Unable to allocate ptLDAPDomainName structure"));
        return FALSE;
    }

    if (memcpy_s(pLDAPConnectInfo->ptLDAPDomainName, (dnsDomainLen + 1) * sizeof(TCHAR), *pLdapServiceName, dnsDomainLen * sizeof(TCHAR))) {
        LOG(Warn, _T("Unable to copy ptLDAPDomainName structure"));
        return FALSE;
    }
    pLDAPConnectInfo->ptLDAPDomainName[dnsDomainLen] = '\0';

    if (plmsgSearchResponse != NULL)
        ldap_msgfree(plmsgSearchResponse);

    if (bIsMissingDnsName)
        HeapFree(hLdapHeap, 0, *pLdapServiceName);

    return TRUE;
}

BOOL LdapPagedSearchFullResult(
    _In_ PLDAP_OPTIONS pLdapOptions,
    _In_ PLDAP_CONNECT_INFO pLdapConnectInfos,
    _In_ LPTSTR ptLdapConnectNC,
    _In_ LPTSTR ptLdapDumpDN,
    _In_ LPTSTR ptLdapFilter,
    _In_ PLDAPControl *pLdapServerControls,
    _In_ const LPTSTR ptLdapAttributesNames[],
    _In_ DWORD dwLdapAttributesCount,
    _In_ PLDAP_FULL_RESULT pLdapFullResults
    ) {
    BOOL bResult = FALSE;

    bResult = LdapBind(pLdapConnectInfos, ptLdapConnectNC, pLdapOptions->tADLogin, pLdapOptions->tADPassword, pLdapOptions->tADExplicitDomain);
    if (!bResult) {
        FATAL(_T("Failed to bind to <%s>"), ptLdapConnectNC);
    }

    pLdapFullResults->dwResultCount = 0;
    pLdapFullResults->pResults = NULL;

    for (DWORD i = 0; i < dwLdapAttributesCount; i++) {

        PLDAP_RETRIEVED_DATA *pRetrievedResults = NULL;
        DWORD dwResultsCount = 0;

        LOG(Dbg, _T("NC <%s> ; Filter <%s> ; Attribute <%s> ; "), ptLdapDumpDN, ptLdapFilter, ptLdapAttributesNames[i]);

        bResult = LdapPagedSearch(pLdapConnectInfos, ptLdapDumpDN, ptLdapFilter, ptLdapAttributesNames[i], pLdapServerControls, &pRetrievedResults, &dwResultsCount);
        if (!bResult) {
            LOG(Err, _T("Paged LDAP search failed <%s : %s>"), ptLdapFilter, ptLdapAttributesNames[i])
                return FALSE;
        }
        LOG(Dbg, SUB_LOG(_T("Results <%u>")), dwResultsCount);

        if (pLdapFullResults->pResults == NULL) {
            LOG(Dbg, _T("New, initialazing..."));
            pLdapFullResults->dwResultCount = dwResultsCount;
            pLdapFullResults->pResults = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(PLDAP_RESULT_ENTRY)* dwResultsCount);
            for (DWORD e = 0; e < dwResultsCount; e++) {
                pLdapFullResults->pResults[e] = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(LDAP_RESULT_ENTRY));
                pLdapFullResults->pResults[e]->pLdapAttrs = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(PLDAP_RESULT_ATTRIBUTE)* dwLdapAttributesCount);
                pLdapFullResults->pResults[e]->dwAttrsCount = dwLdapAttributesCount;
            }
        }
        else {
            // TODO : handle this case normally with a realloc ?
            if (dwResultsCount > pLdapFullResults->dwResultCount) {
                FATAL(_T("attr <%s> count <%u> > <%u>"), ptLdapAttributesNames[i], dwResultsCount, pLdapFullResults->dwResultCount);
            }
        }


        for (DWORD j = 0; j < dwResultsCount; j++) {
            DWORD dwCurrentElementIdx = 0;
            DWORD dwLen = 0;

            if (pRetrievedResults[j]->dwElementCount < 1) {
                FATAL(_T("Attribute <%s> was not retreived for entry <%u:%s> <count=%u>"), ptLdapAttributesNames[i], j, pRetrievedResults[j]->tDN, pRetrievedResults[j]->dwElementCount);
            }

            if (pLdapFullResults->pResults[j]->ptDn == NULL) {
                dwLen = (DWORD)_tcslen(pRetrievedResults[j]->tDN) + 1;
                dwCurrentElementIdx = j;
                pLdapFullResults->pResults[dwCurrentElementIdx]->ptDn = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, dwLen);
                RtlCopyMemory(pLdapFullResults->pResults[dwCurrentElementIdx]->ptDn, pRetrievedResults[j]->tDN, dwLen);
                //DBG LOG(Dbg, "Copying DN : <%u> <%s> (<%s>)", j, pRetrievedResults[j]->tDN, pLdapFullResults->pResults[dwCurrentElementIdx]->ptDn);
            }
            else {
                //DBG LOG(Dbg, "<%u> already has a DN : <%p> <%s>", j, pRetrievedResults[j]->tDN, pRetrievedResults[j]->tDN);
                DWORD idx = 0;
                for (idx = 0; idx < pLdapFullResults->dwResultCount; idx++) {
                    if (pLdapFullResults->pResults[idx] && pLdapFullResults->pResults[idx]->ptDn && _tcscmp(pLdapFullResults->pResults[idx]->ptDn, pRetrievedResults[j]->tDN) == 0) {
                        dwCurrentElementIdx = idx;
                        //DBG LOG(Dbg, _T("Found element <%u:%s>"), idx, pLdapFullResults->pResults[idx]->ptDn);
                        break;
                    }
                }

                if (idx == pLdapFullResults->dwResultCount) {
                    FATAL(_T("Unknown DN <%s>"), pRetrievedResults[j]->tDN);
                }
            }

            pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i] = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(LDAP_RESULT_ATTRIBUTE));
            pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->dwValuesCount = pRetrievedResults[j]->dwElementCount;
            pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->ppbValuesData = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(PBYTE)* pRetrievedResults[j]->dwElementCount);
            pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->pdwValuesSize = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(DWORD)* pRetrievedResults[j]->dwElementCount);
            for (DWORD k = 0; k < pRetrievedResults[j]->dwElementCount; k++) {
                dwLen = pRetrievedResults[j]->pdwDataSize[k];
                pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->pdwValuesSize[k] = dwLen;
                pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->ppbValuesData[k] = HeapAllocCheckX(hLdapHeap, HEAP_ZERO_MEMORY, sizeof(BYTE)* (dwLen + 1));
                RtlCopyMemory(pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->ppbValuesData[k], pRetrievedResults[j]->ppbData[k], dwLen);
                pLdapFullResults->pResults[dwCurrentElementIdx]->pLdapAttrs[i]->ppbValuesData[k][dwLen] = 0;
            }
        }

        if (pRetrievedResults) {
            LdapFreePagedSearchResults(&pRetrievedResults, &dwResultsCount);
        }
    }

    return TRUE;
}

void LdapDestroyResults(
    _Inout_ PLDAP_FULL_RESULT pLdapResults
    ) {
    if (pLdapResults && pLdapResults->dwResultCount > 0) {
        for (DWORD e = 0; e < pLdapResults->dwResultCount; e++) {
            if (pLdapResults->pResults[e]) {
                for (DWORD a = 0; a < pLdapResults->pResults[e]->dwAttrsCount; a++) {
                    if (pLdapResults->pResults[e]->pLdapAttrs[a]) {
                        for (DWORD v = 0; v < pLdapResults->pResults[e]->pLdapAttrs[a]->dwValuesCount; v++) {
                            HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->pLdapAttrs[a]->ppbValuesData[v]);
                        }
                        HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->pLdapAttrs[a]->pdwValuesSize);
                        HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->pLdapAttrs[a]->ppbValuesData);
                        HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->pLdapAttrs[a]);
                    }
                }
                HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->ptDn);
                HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]->pLdapAttrs);
                HeapFree(hLdapHeap, 0, pLdapResults->pResults[e]);
            }
        }
        HeapFree(hLdapHeap, 0, pLdapResults->pResults);
    }
}
