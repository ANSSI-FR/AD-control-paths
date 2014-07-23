/******************************************************************************\
\******************************************************************************/

/* --- INCLUDES ------------------------------------------------------------- */
#include "PluginApi.h"


/* --- PRIVATE VARIABLES ---------------------------------------------------- */
/* --- PUBLIC VARIABLES ----------------------------------------------------- */
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

    .InputFile = {
        .ParseLine = ParseLine,
        .ReadLine = ReadLine,
        .ReadParseTsvLine = ReadParseTsvLine,
        .ForeachLine = ForeachLine,
        .InitInputFile = InitInputFile,
        .ResetInputFile = ResetInputFile,
        .CloseInputFile = CloseInputFile,
    },

    .Alloc = {
        .LocalAllocCheck = LocalAllocCheck,
        .LocalFreeCheck = LocalFreeCheck,
        .HeapAllocCheck = HeapAllocCheck,
        .HeapFreeCheck = HeapFreeCheck,
        .StrDupCheck = StrDupCheck,
        .FreeCheck = FreeCheck,
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
    },

    .Object = {
        .ParseObjectClasses = ParseObjectClasses,
    },

    .Resolver = {
        .ResolverGetAceTrusteeStr = ResolverGetAceTrusteeStr,
        //.ResolverGetObjectPrimaryOwnerStr = ResolverGetObjectPrimaryOwnerStr,
        //.ResolverGetObjectPrimaryGroupStr = ResolverGetObjectPrimaryGroupStr,
        .ResolverGetAceTrustee = ResolverGetAceTrustee,
        .ResolverGetAceObject = ResolverGetAceObject,
        .ResolverGetObjectObjectClass = ResolverGetObjectObjectClass,
        .ResolverGetSchemaDefaultSD = ResolverGetSchemaDefaultSD,
        .ResolverGetSchemaObject = ResolverGetSchemaObject,
        .GetObjectByDn = GetObjectByDn,
        .GetObjectBySid = GetObjectBySid,
        .GetSchemaByGuid = GetSchemaByGuid,
        .GetDomainDn = GetDomainDn,
    }
};

/* --- PRIVATE FUNCTIONS ---------------------------------------------------- */
/* --- PUBLIC FUNCTIONS ----------------------------------------------------- */
