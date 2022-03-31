// Copyright 2020, Roberto De Ioris.

using UnrealBuildTool;

public class glTFRuntime : ModuleRules
{
	public glTFRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrecompileForTargets = PrecompileTargetsType.Any;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
				"ProceduralMeshComponent"
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"JSON",
				"RenderCore",
				"RHI",
				"ApplicationCore",
				"Http",
				"PhysicsCore",
				"Projects"
				// ... add private dependencies that you statically link with here ...	
			}
		);

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.Add("SkeletalMeshUtilitiesCommon");
			PrivateDependencyModuleNames.Add("UnrealEd");
			PrivateDependencyModuleNames.Add("AssetRegistry");
		}
	}
}