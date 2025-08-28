// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECSNET4UNREAL.h"

#define LOCTEXT_NAMESPACE "FECSNET4UNREALModule"

void FECSNET4UNREALModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FECSNET4UNREALModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FECSNET4UNREALModule, ECSNET4UNREAL)