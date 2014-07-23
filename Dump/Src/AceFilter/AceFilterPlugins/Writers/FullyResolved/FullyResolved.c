/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("FullyResolved")
#define PLUGIN_KEYWORD      _T("FRE")
#define PLUGIN_DESCRIPTION  _T("Outputs ACE with a maximum of resolved information");

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
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DN_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_SID_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_CLASSID_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_GUID_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_ADMINSDHOLDER_SD);


/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_FRE_OUTFILE                 _T("freout.tsv")
//#define FRE_OUTFILE_HEADER                  _T("Dn\tPrimaryOwnerSid\tPrimaryOwnerResolved\tPrimaryGroupSid\tPrimaryGroupResolved\tObjectClasses\tObjectClassesResolved\tAdminCount\tTrustee\tTrusteeResolved\tAceType\tAceTypeResolved\tAceFlags\tAceFlagsResolved\tAcessMask\tAccessMaskGeneric\tAccessMaskStandard\tAccessMaskSpecific\tObjectFlags\tObjectFlagsResolved\tObjectType\tObjectTypeResolved\tInheritedObjectType\tInheritedObjectTypeResolved\tDefaultFrom\n")
#define FRE_OUTFILE_HEADER                  _T("Dn\tObjectClasses\tObjectClassesResolved\tAdminCount\tTrustee\tTrusteeResolved\tAceType\tAceTypeResolved\tAceFlags\tAceFlagsResolved\tAcessMask\tAccessMaskGeneric\tAccessMaskStandard\tAccessMaskSpecific\tObjectFlags\tObjectFlagsResolved\tObjectType\tObjectTypeResolved\tInheritedObjectType\tInheritedObjectTypeResolved\tDefaultFrom\n")


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static LPTSTR gs_outfileName = NULL;
static HANDLE gs_hOutfile = INVALID_HANDLE_VALUE;


/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
void StrToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_opt_ LPTSTR str,
    _In_ BOOL end
    ) {
    BOOL bResult = FALSE;
    DWORD written = 0;

    if (!STR_EMPTY(str)) {
        bResult = WriteFile(gs_hOutfile, str, (DWORD)_tcslen(str), &written, NULL);
        if (!bResult) {
            API_LOG(Err, _T("Failed to write <%s> to outfile : <%u>"), str, GetLastError());
        }
    }
    WriteFile(gs_hOutfile, end ? "\n" : "\t", 1, &written, NULL);
}

void IntToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ DWORD value,
    _In_ BOOL hex,
    _In_ BOOL end
    ) {
    TCHAR strvalue[10 + 1] = { 0 }; // hex:0x + 8, dec:10
    _sntprintf_s(strvalue, _countof(strvalue), 10, hex ? _T("%#x") : _T("%u"), value);
    StrToOutfile(api, strvalue, end);
}

void EnumToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ DWORD value,
    _In_ NUMERIC_CONSTANT const * const values,
    _In_ DWORD count,
    _In_ BOOL end
    ) {
    DWORD i = 0;

    for (i = 0; i < count; i++) {
        if (value == values[i].value) {
            StrToOutfile(api, values[i].name, end);
            return;
        }
    }

    API_LOG(Err, _T("Unknow enum value %u"), value);
    StrToOutfile(api, _T("<Error>"), end);
}

void FlagsToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ DWORD mask,
    _In_ NUMERIC_CONSTANT const * const flags,
    _In_ DWORD count,
    _In_ BOOL end
    ) {
    DWORD i = 0, n = 0;
    size_t len = 0;
    LPTSTR str = NULL;

    for (i = 0; i < count; i++) {
        if ((mask & flags[i].value) > 0) {
            len += _tcslen(flags[i].name) + 1;
        }
    }

    str = ApiLocalAllocCheckX(len);

    for (i = 0; i < count; i++) {
        if ((mask & flags[i].value) > 0) {
            if (n > 0) {
                StringCchCat(str, len, _T(","));
            }
            StringCchCat(str, len, flags[i].name);
            n++;
        }
    }

    StrToOutfile(api, str, end);
    ApiLocalFreeCheckX(str);
}

void SidToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ PSID sid,
    _In_ BOOL end
    ) {
    LPTSTR strsid = NULL;
    BOOL bResult = ConvertSidToStringSid(sid, &strsid);
    if (!bResult) {
        API_LOG(Err, _T("Failed to stringify SID : <%u>"), GetLastError());
        StrToOutfile(api, _T("<Error>"), end);
        return;
    }
    StrToOutfile(api, strsid, end);
    LocalFree(strsid);
}

void GuidToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
    _In_ GUID * guid,
    _In_ BOOL end
    ) {
    LPOLESTR str = NULL;
    HRESULT hr = StringFromCLSID(guid, &str);
    if (hr == S_OK) {
#ifdef _UNICODE
        StrToOutfile(api, str, end);
#else
        size_t n = 0;
        CHAR converted[GUID_STR_SIZE_NULL] = { 0 };
        wcstombs_s(&n, converted, GUID_STR_SIZE_NULL, str, GUID_STR_SIZE_NULL - 1);
        StrToOutfile(api, converted, FALSE);
#endif
        CoTaskMemFree(str);

        PIMPORTED_SCHEMA schema = api->Resolver.GetSchemaByGuid(guid);
        StrToOutfile(api, schema ? schema->imported.dn : NULL, end);
    }
    else {
        API_LOG(Err, _T("Failed to stringify GUID"));
        StrToOutfile(api, _T("<Error>"), end);
        StrToOutfile(api, _T("<Error>"), end);
    }
}

/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
void PLUGIN_GENERIC_HELP(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    API_LOG(Bypass, _T("Specify the outfile with the <freout> plugin option (default <%s>)"), DEFAULT_FRE_OUTFILE);
}

BOOL PLUGIN_GENERIC_INITIALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
    DWORD dwWritten = 0;
    BOOL bResult = FALSE;

    gs_outfileName = api->Common.GetPluginOption(_T("freout"), FALSE);
    if (!gs_outfileName) {
        gs_outfileName = DEFAULT_FRE_OUTFILE;
    }
    API_LOG(Info, _T("Outfile is <%s>"), gs_outfileName);


    gs_hOutfile = CreateFile(gs_outfileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (gs_hOutfile == INVALID_HANDLE_VALUE) {
        API_FATAL(_T("Failed to create outfile <%s> : <%u>"), gs_outfileName, GetLastError());
    }
    bResult = WriteFile(gs_hOutfile, FRE_OUTFILE_HEADER, (DWORD)_tcslen(FRE_OUTFILE_HEADER), &dwWritten, NULL);
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

    /********************\
    | Object information |
    \********************/
    PIMPORTED_OBJECT obj = api->Resolver.ResolverGetAceObject(ace);
    if (!obj) {
        API_LOG(Err, _T("Cannot lookup object <%s> for ace <%u>. Skipping."), ace->imported.objectDn, ace->computed.number);
        return FALSE;
    }

    //
    // Dn
    //
    StrToOutfile(api, obj->imported.dn, FALSE);

    //
    // PrimaryOwnerSid, PrimaryOwnerResolved
    //
    /*
    if (obj->imported.primaryOwner) {
    //DBG
    API_LOG(Dbg, _T("- owner <%u>"), IsValidSid(obj->imported.primaryOwner));
    //DBG
    SidToOutfile(api, obj->imported.primaryOwner, FALSE);
    StrToOutfile(api, api->Resolver.ResolverGetObjectPrimaryOwnerStr(obj), FALSE);
    }
    else {
    StrToOutfile(api, NULL, FALSE);
    StrToOutfile(api, NULL, FALSE);
    }
    */

    //
    // PrimaryGroupSid, PrimaryGroupResolved
    //
    /*
    if (obj->imported.primaryGroup) {
    //DBG
    API_LOG(Dbg, _T("- group <%u>"), IsValidSid(obj->imported.primaryGroup));
    //DBG
    SidToOutfile(api, obj->imported.primaryGroup, FALSE);
    StrToOutfile(api, api->Resolver.ResolverGetObjectPrimaryGroupStr(obj), FALSE);
    }
    else {
    StrToOutfile(api, NULL, FALSE);
    StrToOutfile(api, NULL, FALSE);
    }
    */

    //
    // ObjectClasses, ObjectClassesResolved
    //

    // TODO : poorly written...
    if (obj->computed.objectClassCount > 0) {
        DWORD i = 0;
        size_t size = obj->computed.objectClassCount * (8 + 1);
        int ret = 0, written = 0;
        LPTSTR classIds = ApiLocalAllocCheckX(size);

        ret = _sntprintf_s(classIds, size, size - 1, _T("%08x"), obj->imported.objectClassesIds[0]);
        if (ret != -1) {
            written = ret;
            for (i = 1; i < obj->computed.objectClassCount; i++) {
                ret = _sntprintf_s(classIds + written, size - written, size - written - 1, _T(",%08x"), obj->imported.objectClassesIds[i]);
                if (ret == -1) {
                    break;
                }
                written += ret;
            }
        }
        StrToOutfile(api, classIds, FALSE);
        ApiLocalFreeCheckX(classIds);
        classIds = NULL;

        LPTSTR className = NULL, classDn = NULL, ctx = NULL;
        PIMPORTED_SCHEMA schemaClass = NULL;
        DWORD bytesWritten = 0;
        written = 0;
        for (i = 0; i < obj->computed.objectClassCount; i++) {
            classDn = className = ctx = NULL;
            schemaClass = api->Resolver.ResolverGetObjectObjectClass(obj, i);
            if (schemaClass) {
                classDn = ApiStrDupCheckX(schemaClass->imported.dn);
                api->Common.StrNextToken(classDn + 3, _T(","), &ctx, &className);
            }
            else {
                API_LOG(Err, _T("Failed to lookup class <%u/%u> for <%s>"), i + 1, obj->computed.objectClassCount, obj->imported.dn);
                className = _T("<Error>");
            }
            if (written > 0) {
                WriteFile(gs_hOutfile, _T(","), (DWORD)_tcslen(_T(",")), (PDWORD)&bytesWritten, NULL);
            }
            WriteFile(gs_hOutfile, className, (DWORD)_tcslen(className), (PDWORD)&bytesWritten, NULL);
            ApiFreeCheckX(classDn);
            written++;
        }
        StrToOutfile(api, NULL, FALSE);
    }
    else {
        StrToOutfile(api, NULL, FALSE);
        StrToOutfile(api, NULL, FALSE);
    }

    //
    // AdminCount
    //
    IntToOutfile(api, obj->imported.adminCount, FALSE, FALSE);


    /*****************\
    | ACE information |
    \*****************/

    //
    // Trustee,TrusteeResolved
    //
    SidToOutfile(api, api->Ace.GetTrustee(ace), FALSE);
    StrToOutfile(api, api->Resolver.ResolverGetAceTrusteeStr(ace), FALSE);

    //
    // AceType, AceTypeResolved
    //
    IntToOutfile(api, ace->imported.raw->AceType, TRUE, FALSE);
    EnumToOutfile(api, ace->imported.raw->AceType, gc_AceTypeValues, ARRAY_COUNT(gc_AceTypeValues), FALSE);

    //
    // AceFlags, AceFlagsResolved
    //
    BYTE aceFlags = ace->imported.raw->AceFlags;
    IntToOutfile(api, aceFlags, TRUE, FALSE);
    FlagsToOutfile(api, aceFlags, gc_AceFlagsValues, ARRAY_COUNT(gc_AceFlagsValues), FALSE);

    //
    // AcessMask, AccessMaskGeneric, AccessMaskStandard, AccessMaskSpecific
    //
    DWORD accessMask = api->Ace.GetAccessMask(ace);
    IntToOutfile(api, accessMask, TRUE, FALSE);
    FlagsToOutfile(api, accessMask & ACE_GENERIC_RIGHTS_MASK, gc_AceGenericAccessMaskValues, ARRAY_COUNT(gc_AceGenericAccessMaskValues), FALSE);
    FlagsToOutfile(api, accessMask & ACE_STANDARD_RIGHTS_MASK, gc_AceStandardAccessMaskValues, ARRAY_COUNT(gc_AceStandardAccessMaskValues), FALSE);
    switch (ace->imported.source) {
    case AceFromActiveDirectory: FlagsToOutfile(api, accessMask & ACE_SPECIFIC_RIGHTS_MASK, gc_AceSpecificAdAccessMaskValues, ARRAY_COUNT(gc_AceSpecificAdAccessMaskValues), FALSE); break;
    case AceFromFileSystem:      FlagsToOutfile(api, accessMask & ACE_SPECIFIC_RIGHTS_MASK, gc_AceSpecificFsAccessMaskValues, ARRAY_COUNT(gc_AceSpecificFsAccessMaskValues), FALSE); break;
    default:
        API_LOG(Err, _T("Unknown ACE source <%u>"), ace->imported.source);
    }

    //
    // ObjectFlags,ObjectFlagsResolved
    //
    DWORD objectFlags = IS_OBJECT_ACE(ace->imported.raw) ? api->Ace.GetObjectFlags(ace) : 0;
    IntToOutfile(api, objectFlags, FALSE, FALSE);
    FlagsToOutfile(api, objectFlags, gc_AceObjectFlagsValues, ARRAY_COUNT(gc_AceObjectFlagsValues), FALSE);

    //
    // ObjectType, ObjectTypeResolved
    //
    if (objectFlags & ACE_OBJECT_TYPE_PRESENT) {
        GuidToOutfile(api, api->Ace.GetObjectTypeAce(ace), FALSE);
    }
    else {
        StrToOutfile(api, NULL, FALSE);
        StrToOutfile(api, NULL, FALSE);
    }

    //
    // InheritedObjectType, InheritedObjectTypeResolved
    //
    if (objectFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
        GuidToOutfile(api, api->Ace.GetInheritedObjectTypeAce(ace), FALSE);
    }
    else {
        StrToOutfile(api, NULL, FALSE);
        StrToOutfile(api, NULL, FALSE);
    }

    //
    // DefaultFrom
    //
    if (obj->imported.adminCount == 1 && api->Ace.IsInAdminSdHolder(ace)) {
        StrToOutfile(api, _T("AdminSdHolder"), TRUE);
    }
    else if (api->Ace.IsInDefaultSd(ace)) {
        StrToOutfile(api, _T("DefaultSd"), TRUE);
    }
    else {
        StrToOutfile(api, NULL, TRUE);
    }

    return TRUE;
}
