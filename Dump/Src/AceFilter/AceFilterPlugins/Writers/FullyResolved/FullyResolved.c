/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "..\..\..\AceFilterCore\PluginCommon.h"


/* --- PLUGIN DECLARATIONS -------------------------------------------------- */
#define PLUGIN_NAME         _T("FullyResolved")
#define PLUGIN_KEYWORD      _T("FRE")
#define PLUGIN_DESCRIPTION  _T("Outputs ACE with a maximum of resolved information")
#define FULLYRESOLVED_HEAP_NAME _T("FRHEAP")


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
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_GUID_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_DISPLAYNAME_RESOLUTION);
PLUGIN_DECLARE_REQUIREMENT(PLUGIN_REQUIRE_ADMINSDHOLDER_SD);


/* --- DEFINES -------------------------------------------------------------- */
#define DEFAULT_FRE_OUTFILE                 _T("freout.tsv")
//#define FRE_OUTFILE_HEADER                  _T("Dn\tPrimaryOwnerSid\tPrimaryOwnerResolved\tPrimaryGroupSid\tPrimaryGroupResolved\tObjectClasses\tObjectClassesResolved\tAdminCount\tTrustee\tTrusteeResolved\tAceType\tAceTypeResolved\tAceFlags\tAceFlagsResolved\tAcessMask\tAccessMaskGeneric\tAccessMaskStandard\tAccessMaskSpecific\tObjectFlags\tObjectFlagsResolved\tObjectType\tObjectTypeResolved\tInheritedObjectType\tInheritedObjectTypeResolved\tDefaultFrom\n")
#define FRE_OUTFILE_HEADER                  {_T("dn"),_T("adminCount"),_T("trustee"),_T("trusteeResolved"),_T("aceType"),_T("aceTypeResolved"),_T("aceFlags"),_T("aceFlagsResolved"),_T("accessMask"),_T("accessMaskGeneric"),_T("accessMaskStandard"),_T("accessMaskSpecific"),_T("objectFlags"),_T("objectFlagsResolved"),_T("objectType"),_T("objectTypeResolved"),_T("inheritedObjectType"),_T("inheritedObjectTypeResolved"),_T("defaultFrom")}
#define FRE_OUTFILE_HEADER_COUNT            (19)


/* --- TYPES ---------------------------------------------------------------- */
typedef enum _FR_FIELD_NAME {
	dn = 0,
	adminCount,
	trustee,
	trusteeResolved,
	aceType,
	aceTypeResolved,
	aceFlags,
	aceFlagsResolved,
	accessMask,
	accessMaskGeneric,
	accessMaskStandard,
	accessMaskSpecific,
	objectFlags,
	objectFlagsResolved,
	objectType,
	objectTypeResolved,
	inheritedObjectType,
	inheritedObjectTypeResolved,
	defaultFrom,
} FR_FIELD_NAME, *PFR_FIELD_NAME;

/* --- PRIVATE VARIABLES ---------------------------------------------------- */
static PUTILS_HEAP gs_hHeapFullyResolved = INVALID_HANDLE_VALUE;
static LPTSTR gs_outfileName = NULL;
static CSV_HANDLE gs_hOutfile = 0;

/* --- PUBLIC VARIABLES ----------------------------------------------------- */
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
void IntToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
	_Inout_ LPTSTR ** nextRecord,
	_In_ const FR_FIELD_NAME position,
    _In_ DWORD value,
    _In_ BOOL hex
    ) {
	LPTSTR strvalue = NULL;
	UNREFERENCED_PARAMETER(api);

	strvalue = ApiHeapAllocX(gs_hHeapFullyResolved, 11 * sizeof(TCHAR)); // hex:0x + 8, dec:10
    _sntprintf_s(strvalue, 11, 10, hex ? _T("%#x") : _T("%u"), value);
	(*nextRecord)[position] = strvalue;
}

void EnumToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
	_Inout_ LPTSTR ** nextRecord,
	_In_ const FR_FIELD_NAME position,
    _In_ DWORD value,
    _In_ NUMERIC_CONSTANT const * const values,
    _In_ DWORD count
    ) {
    DWORD i = 0;

    for (i = 0; i < count; i++) {
        if (value == values[i].value) {
			(*nextRecord)[position] = ApiStrDupX(gs_hHeapFullyResolved, values[i].name);
            return;
        }
    }

    API_LOG(Err, _T("Unknow enum value %u"), value);
	(*nextRecord)[position] = ApiStrDupX(gs_hHeapFullyResolved, _T("<Error>"));
}

void FlagsToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
	_Inout_ LPTSTR **nextRecord,
	_In_ const FR_FIELD_NAME position,
    _In_ DWORD mask,
    _In_ NUMERIC_CONSTANT const * const flags,
    _In_ DWORD count
    ) {
    DWORD i = 0, n = 0;
    size_t len = 1;
    LPTSTR str = NULL;

    for (i = 0; i < count; i++) {
        if ((mask & flags[i].value) > 0) {
            len += _tcslen(flags[i].name) + 1;
        }
    }

    str = ApiHeapAllocX(gs_hHeapFullyResolved,(DWORD)(len * sizeof(TCHAR)));
    for (i = 0; i < count; i++) {
        if ((mask & flags[i].value) > 0) {
            if (n > 0) {
                StringCchCat(str, len, _T(","));
            }
            StringCchCat(str, len, flags[i].name);
            n++;
        }
    }
	(*nextRecord)[position] = str;
}

void SidToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
	_Inout_ LPTSTR ** nextRecord,
	_In_ const FR_FIELD_NAME position,
    _In_ PSID sid
    ) {
    LPTSTR strsid = NULL;
	LPTSTR printableSid = NULL;
	BOOL bResult = FALSE;

    bResult = ConvertSidToStringSid(sid, &strsid);
    if (!bResult) {
        API_LOG(Err, _T("Failed to stringify SID : <%u>"), GetLastError());
		(*nextRecord)[position] = ApiStrDupX(gs_hHeapFullyResolved, _T("<Error>"));
        return;
    }

	printableSid = ApiStrDupX(gs_hHeapFullyResolved, strsid);
	LocalFree(strsid);
	(*nextRecord)[position] = printableSid;
}

void GuidToOutfile(
    _In_ PLUGIN_API_TABLE const * const api,
	_Inout_ LPTSTR ** nextRecord,
	_In_ const FR_FIELD_NAME position,
    _In_ GUID * guid
    ) {
    LPOLESTR str = NULL;
	LPTSTR printableGuid = NULL;
	PIMPORTED_SCHEMA schema = NULL;

    HRESULT hr = StringFromCLSID(guid, &str);
    if (hr == S_OK) {
		printableGuid = ApiStrDupX(gs_hHeapFullyResolved, str);
		CoTaskMemFree(str);
		(*nextRecord)[position] = printableGuid;

        schema = api->Resolver.GetSchemaByGuid(guid);
		(*nextRecord)[position + 1] = schema ? ApiStrDupX(gs_hHeapFullyResolved, schema->imported.dn) : ApiStrDupX(gs_hHeapFullyResolved, _T("<Error>"));
    }
    else {
        API_LOG(Err, _T("Failed to stringify GUID"));
		(*nextRecord)[position] = ApiStrDupX(gs_hHeapFullyResolved, _T("<Error>"));
		(*nextRecord)[position + 1] = ApiStrDupX(gs_hHeapFullyResolved, _T("<Error>"));
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
    BOOL bResult = FALSE;
	PTCHAR pptAttrsListForCsv[FRE_OUTFILE_HEADER_COUNT] = FRE_OUTFILE_HEADER;

	bResult = ApiHeapCreateX(&gs_hHeapFullyResolved, FULLYRESOLVED_HEAP_NAME, NULL);
	if (API_FAILED(bResult)) {
		return ERROR_VALUE;
	}

    gs_outfileName = api->Common.GetPluginOption(_T("freout"), FALSE);
    if (!gs_outfileName) {
        gs_outfileName = DEFAULT_FRE_OUTFILE;
    }
    API_LOG(Info, _T("Outfile is <%s>"), gs_outfileName);

	bResult = api->InputCsv.CsvOpenWrite(gs_outfileName, FRE_OUTFILE_HEADER_COUNT, pptAttrsListForCsv, &gs_hOutfile);
	if (!bResult) {
		API_FATAL(_T("Failed to open CSV outfile <%s>: <err:%#08x>"), gs_outfileName, api->InputCsv.CsvGetLastError(gs_hOutfile));
	}
    return TRUE;
}

BOOL PLUGIN_GENERIC_FINALIZE(
    _In_ PLUGIN_API_TABLE const * const api
    ) {
	BOOL bResult = FALSE;

	bResult = api->InputCsv.CsvClose(&gs_hOutfile);
	if (API_FAILED(bResult)) {
		API_FATAL(_T("Failed to close CSV outfile <%s>: <err:%#08x>"), gs_outfileName, api->InputCsv.CsvGetLastError(gs_hOutfile));
	}

	bResult = ApiHeapDestroyX(&gs_hHeapFullyResolved);
	if (API_FAILED(bResult)) {
		API_LOG(Err, _T("Failed to destroy heap: <%u>"), GetLastError());
		return ERROR_VALUE;
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
	PIMPORTED_OBJECT obj = NULL;
	LPTSTR *nextRecord = NULL;

	nextRecord = ApiHeapAllocX(gs_hHeapFullyResolved, FRE_OUTFILE_HEADER_COUNT * sizeof(LPTSTR));
	obj = api->Resolver.ResolverGetAceObject(ace);
    if (!obj) {
        API_LOG(Err, _T("Cannot lookup object <%s> for ace <%u>. Skipping."), ace->imported.objectDn, ace->computed.number);
        return FALSE;
    }

    //
    // Dn
    //
    nextRecord[dn] = ApiStrDupX(gs_hHeapFullyResolved,obj->imported.dn);

    // TODO : ObjectClass

    //
    // AdminCount
    //
    IntToOutfile(api, &nextRecord, adminCount, obj->imported.adminCount, FALSE);


    /*****************\
    | ACE information |
    \*****************/

    //
    // Trustee,TrusteeResolved
    //
    SidToOutfile(api, &nextRecord, trustee, api->Ace.GetTrustee(ace));
    nextRecord[trusteeResolved] = ApiStrDupX(gs_hHeapFullyResolved, api->Resolver.ResolverGetAceTrusteeStr(ace));

    //
    // AceType, AceTypeResolved
    //
    IntToOutfile(api, &nextRecord, aceType, ace->imported.raw->AceType, TRUE);
	EnumToOutfile(api, &nextRecord, aceTypeResolved, ace->imported.raw->AceType, gc_AceTypeValues, ARRAY_COUNT(gc_AceTypeValues));

    //
    // AceFlags, AceFlagsResolved
    //
    BYTE aceFlagsByte = ace->imported.raw->AceFlags;
    IntToOutfile(api, &nextRecord, aceFlags, aceFlagsByte, TRUE);
    FlagsToOutfile(api, &nextRecord, aceFlagsResolved, aceFlagsByte, gc_AceFlagsValues, ARRAY_COUNT(gc_AceFlagsValues));

    //
    // AcessMask, AccessMaskGeneric, AccessMaskStandard, AccessMaskSpecific
    //
    DWORD dwAccessMask = api->Ace.GetAccessMask(ace);
    IntToOutfile(api, &nextRecord, accessMask, dwAccessMask, TRUE);
    FlagsToOutfile(api, &nextRecord, accessMaskGeneric, dwAccessMask & ACE_GENERIC_RIGHTS_MASK, gc_AceGenericAccessMaskValues, ARRAY_COUNT(gc_AceGenericAccessMaskValues));
    FlagsToOutfile(api, &nextRecord, accessMaskStandard, dwAccessMask & ACE_STANDARD_RIGHTS_MASK, gc_AceStandardAccessMaskValues, ARRAY_COUNT(gc_AceStandardAccessMaskValues));
    switch (ace->imported.source) {
    case AceFromActiveDirectory: FlagsToOutfile(api, &nextRecord, accessMaskSpecific, dwAccessMask & ACE_SPECIFIC_RIGHTS_MASK, gc_AceSpecificAdAccessMaskValues, ARRAY_COUNT(gc_AceSpecificAdAccessMaskValues)); break;
    case AceFromFileSystem:      FlagsToOutfile(api, &nextRecord, accessMaskSpecific, dwAccessMask & ACE_SPECIFIC_RIGHTS_MASK, gc_AceSpecificFsAccessMaskValues, ARRAY_COUNT(gc_AceSpecificFsAccessMaskValues)); break;
    default:
        API_LOG(Err, _T("Unknown ACE source <%u>"), ace->imported.source);
    }

    //
    // ObjectFlags,ObjectFlagsResolved
    //
    DWORD dwObjectFlags = IS_OBJECT_ACE(ace->imported.raw) ? api->Ace.GetObjectFlags(ace) : 0;
    IntToOutfile(api, &nextRecord, objectFlags, dwObjectFlags, FALSE);
    FlagsToOutfile(api, &nextRecord, objectFlagsResolved, dwObjectFlags, gc_AceObjectFlagsValues, ARRAY_COUNT(gc_AceObjectFlagsValues));

    //
    // ObjectType, ObjectTypeResolved
    //
    if (dwObjectFlags & ACE_OBJECT_TYPE_PRESENT) {
        GuidToOutfile(api, &nextRecord, objectType, api->Ace.GetObjectTypeAce(ace));
    }
	else {
		nextRecord[objectType] = ApiStrDupX(gs_hHeapFullyResolved, _T(""));
		nextRecord[objectTypeResolved] = ApiStrDupX(gs_hHeapFullyResolved, _T(""));
	}


    //
    // InheritedObjectType, InheritedObjectTypeResolved
    //
    if (dwObjectFlags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
        GuidToOutfile(api, &nextRecord, inheritedObjectType, api->Ace.GetInheritedObjectTypeAce(ace));
    }
	else {
		nextRecord[inheritedObjectType] = ApiStrDupX(gs_hHeapFullyResolved, _T(""));
		nextRecord[inheritedObjectTypeResolved] = ApiStrDupX(gs_hHeapFullyResolved, _T(""));
	}

    //
    // DefaultFrom
    //
    if (obj->imported.adminCount == 1 && api->Ace.IsInAdminSdHolder(ace)) {
		nextRecord[defaultFrom] = ApiStrDupX(gs_hHeapFullyResolved, _T("AdminSdHolder"));
    }
    else if (api->Ace.IsInDefaultSd(ace)) {
		nextRecord[defaultFrom] = ApiStrDupX(gs_hHeapFullyResolved, _T("DefaultSd"));
    }
	else {
		nextRecord[defaultFrom] = ApiStrDupX(gs_hHeapFullyResolved, _T(""));
	}

	//
	// Write the whole record
	//
	api->InputCsv.CsvWriteNextRecord(gs_hOutfile, nextRecord, NULL);
	ApiHeapFreeArrayX(gs_hHeapFullyResolved, nextRecord, FRE_OUTFILE_HEADER_COUNT);

    return TRUE;
}
