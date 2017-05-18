/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "PluginApi.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
// As we do a jump table from non static dllimports
#pragma warning(disable:4232)
PLUGIN_API_TABLE const gc_PluginApiTable = {
	.Common = {
		.Log = Log, // Do not use directly (use the LOG() macro implicitely using this instead)
		.GetPluginOption = GetPluginOption,
		.ConvertStrGuidToGuid = ConvertStrGuidToGuid,
		.StrNextToken = StrNextToken,
		.IsInSetOfStrings = IsInSetOfStrings,
		.Hexify = Hexify,
		.Unhexify = Unhexify,
		.EnablePrivForCurrentProcess = EnablePrivForCurrentProcess,
	},

	.InputCsv = {
		.CsvOpenRead = CsvOpenRead,
		.CsvGetNextRecord = CsvGetNextRecord,
		.CsvClose = CsvClose,
		.CsvResetFile = CsvResetFile,
		.CsvHeapFree = CsvHeapFree,
		.CsvRecordArrayHeapFree = CsvRecordArrayHeapFree,
		.CsvOpenWrite = CsvOpenWrite,
		.CsvWriteNextRecord = CsvWriteNextRecord,
		.CsvGetLastError = CsvGetLastError,
},

    .Alloc = {
        .UtilsHeapAlloc = UtilsHeapAlloc,
        .UtilsHeapFree = UtilsHeapFree,
        .UtilsHeapStrDup = UtilsHeapStrDup,
		.UtilsHeapFreeArray = UtilsHeapFreeArray,
		.UtilsHeapCreate = UtilsHeapCreate,
		.UtilsHeapDestroy = UtilsHeapDestroy,
    },

    .Ace = {
			.GetAccessMask = GetAccessMask,
			.GetTrustee = GetTrustee,
			.GetObjectFlags = GetObjectFlags,
			.GetObjectTypeAce = GetObjectTypeAce,
			.GetInheritedObjectTypeAce = GetInheritedObjectTypeAce,
			.GetAceRelationStr = GetAceRelationStr,
			.IsAceInSd = IsAceInSd,
			.IsInAdminSdHolder = IsInAdminSdHolder,
			.IsInDefaultSd = IsInDefaultSd,
			.isObjectTypeClassMatching = isObjectTypeClassMatching,
			.isObjectTypeClass = isObjectTypeClass,
    },

    .Resolver = {
        .ResolverGetAceTrusteeStr = ResolverGetAceTrusteeStr,
        .ResolverGetAceTrustee = ResolverGetAceTrustee,
        .ResolverGetAceObject = ResolverGetAceObject,
		.ResolverGetAceObjectMail = ResolverGetAceObjectMail,
        .ResolverGetSchemaDefaultSD = ResolverGetSchemaDefaultSD,
        .ResolverGetSchemaObject = ResolverGetSchemaObject,
        .GetObjectByDn = GetObjectByDn,
        .GetObjectBySid = GetObjectBySid,
        .GetSchemaByGuid = GetSchemaByGuid,
        .GetDomainDn = GetDomainDn,
		.GetAdmPwdGuid = GetAdmPwdGuid,
    }
};
#pragma warning(default:4232)
/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
