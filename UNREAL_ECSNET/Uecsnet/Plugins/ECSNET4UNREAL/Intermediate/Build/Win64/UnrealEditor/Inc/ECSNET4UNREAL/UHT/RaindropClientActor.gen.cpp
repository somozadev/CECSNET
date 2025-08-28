// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ECSNET4UNREAL/Public/RaindropClientActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeRaindropClientActor() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
ECSNET4UNREAL_API UClass* Z_Construct_UClass_ARaindropClientActor();
ECSNET4UNREAL_API UClass* Z_Construct_UClass_ARaindropClientActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
UPackage* Z_Construct_UPackage__Script_ECSNET4UNREAL();
// End Cross Module References

// Begin Class ARaindropClientActor
void ARaindropClientActor::StaticRegisterNativesARaindropClientActor()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ARaindropClientActor);
UClass* Z_Construct_UClass_ARaindropClientActor_NoRegister()
{
	return ARaindropClientActor::StaticClass();
}
struct Z_Construct_UClass_ARaindropClientActor_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "RaindropClientActor.h" },
		{ "ModuleRelativePath", "Public/RaindropClientActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ServerIp_MetaData[] = {
		{ "Category", "RaindropClientActor" },
		{ "ModuleRelativePath", "Public/RaindropClientActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TcpPort_MetaData[] = {
		{ "Category", "RaindropClientActor" },
		{ "ModuleRelativePath", "Public/RaindropClientActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UdpPort_MetaData[] = {
		{ "Category", "RaindropClientActor" },
		{ "ModuleRelativePath", "Public/RaindropClientActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_DropClass_MetaData[] = {
		{ "Category", "RaindropClientActor" },
		{ "ModuleRelativePath", "Public/RaindropClientActor.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ServerIp;
	static const UECodeGen_Private::FIntPropertyParams NewProp_TcpPort;
	static const UECodeGen_Private::FIntPropertyParams NewProp_UdpPort;
	static const UECodeGen_Private::FClassPropertyParams NewProp_DropClass;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ARaindropClientActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_ServerIp = { "ServerIp", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARaindropClientActor, ServerIp), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ServerIp_MetaData), NewProp_ServerIp_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_TcpPort = { "TcpPort", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARaindropClientActor, TcpPort), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TcpPort_MetaData), NewProp_TcpPort_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_UdpPort = { "UdpPort", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARaindropClientActor, UdpPort), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UdpPort_MetaData), NewProp_UdpPort_MetaData) };
const UECodeGen_Private::FClassPropertyParams Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_DropClass = { "DropClass", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ARaindropClientActor, DropClass), Z_Construct_UClass_UClass, Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_DropClass_MetaData), NewProp_DropClass_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ARaindropClientActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_ServerIp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_TcpPort,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_UdpPort,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ARaindropClientActor_Statics::NewProp_DropClass,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ARaindropClientActor_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_ARaindropClientActor_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_ECSNET4UNREAL,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ARaindropClientActor_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ARaindropClientActor_Statics::ClassParams = {
	&ARaindropClientActor::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ARaindropClientActor_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ARaindropClientActor_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ARaindropClientActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ARaindropClientActor_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_ARaindropClientActor()
{
	if (!Z_Registration_Info_UClass_ARaindropClientActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ARaindropClientActor.OuterSingleton, Z_Construct_UClass_ARaindropClientActor_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ARaindropClientActor.OuterSingleton;
}
template<> ECSNET4UNREAL_API UClass* StaticClass<ARaindropClientActor>()
{
	return ARaindropClientActor::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(ARaindropClientActor);
ARaindropClientActor::~ARaindropClientActor() {}
// End Class ARaindropClientActor

// Begin Registration
struct Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_RaindropClientActor_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ARaindropClientActor, ARaindropClientActor::StaticClass, TEXT("ARaindropClientActor"), &Z_Registration_Info_UClass_ARaindropClientActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ARaindropClientActor), 1343782042U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_RaindropClientActor_h_2704050682(TEXT("/Script/ECSNET4UNREAL"),
	Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_RaindropClientActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_RaindropClientActor_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
