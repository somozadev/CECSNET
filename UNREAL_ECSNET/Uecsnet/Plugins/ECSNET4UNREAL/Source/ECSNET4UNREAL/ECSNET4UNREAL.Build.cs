// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ECSNET4UNREAL : ModuleRules
{
	public ECSNET4UNREAL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore", 
				"InputCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty/ecsnet");
		string IncludePath    = Path.Combine(ThirdPartyPath, "include");
		string LibPath        = Path.Combine(ThirdPartyPath, "lib");

		PublicIncludePaths.Add(IncludePath);

		PublicAdditionalLibraries.Add(Path.Combine(LibPath, "ecsnet.lib"));

		PublicDefinitions.Add("ECSNET_API=");
	}
}
