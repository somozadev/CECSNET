// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ECSNET4UNREAL/Public/DropActor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeDropActor() {}

// Begin Cross Module References
ECSNET4UNREAL_API UClass* Z_Construct_UClass_ADropActor();
ECSNET4UNREAL_API UClass* Z_Construct_UClass_ADropActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_AActor();
ENGINE_API UClass* Z_Construct_UClass_UStaticMeshComponent_NoRegister();
UPackage* Z_Construct_UPackage__Script_ECSNET4UNREAL();
// End Cross Module References

// Begin Class ADropActor
void ADropActor::StaticRegisterNativesADropActor()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ADropActor);
UClass* Z_Construct_UClass_ADropActor_NoRegister()
{
	return ADropActor::StaticClass();
}
struct Z_Construct_UClass_ADropActor_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "DropActor.h" },
		{ "ModuleRelativePath", "Public/DropActor.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Mesh_MetaData[] = {
		{ "Category", "DropActor" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/DropActor.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Mesh;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ADropActor>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_ADropActor_Statics::NewProp_Mesh = { "Mesh", nullptr, (EPropertyFlags)0x00100000000a001d, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ADropActor, Mesh), Z_Construct_UClass_UStaticMeshComponent_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Mesh_MetaData), NewProp_Mesh_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ADropActor_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ADropActor_Statics::NewProp_Mesh,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ADropActor_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_ADropActor_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AActor,
	(UObject* (*)())Z_Construct_UPackage__Script_ECSNET4UNREAL,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ADropActor_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ADropActor_Statics::ClassParams = {
	&ADropActor::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ADropActor_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ADropActor_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ADropActor_Statics::Class_MetaDataParams), Z_Construct_UClass_ADropActor_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_ADropActor()
{
	if (!Z_Registration_Info_UClass_ADropActor.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ADropActor.OuterSingleton, Z_Construct_UClass_ADropActor_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ADropActor.OuterSingleton;
}
template<> ECSNET4UNREAL_API UClass* StaticClass<ADropActor>()
{
	return ADropActor::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(ADropActor);
ADropActor::~ADropActor() {}
// End Class ADropActor

// Begin Registration
struct Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_DropActor_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ADropActor, ADropActor::StaticClass, TEXT("ADropActor"), &Z_Registration_Info_UClass_ADropActor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ADropActor), 2807705661U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_DropActor_h_1320521592(TEXT("/Script/ECSNET4UNREAL"),
	Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_DropActor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_mesom_Documents_GitHub_CECSNET_UNREAL_ECSNET_Uecsnet_Plugins_ECSNET4UNREAL_Source_ECSNET4UNREAL_Public_DropActor_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
