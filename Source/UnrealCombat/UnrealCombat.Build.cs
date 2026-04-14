// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealCombat : ModuleRules
{
	public UnrealCombat(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"MotionWarping",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"UnrealCombat",
			"UnrealCombat/Variant_Combat",
			"UnrealCombat/Variant_Combat/Abilities",
			"UnrealCombat/Variant_Combat/AI",
			"UnrealCombat/Variant_Combat/Animation",
			"UnrealCombat/Variant_Combat/Gameplay",
			"UnrealCombat/Variant_Combat/Interfaces",
			"UnrealCombat/Variant_Combat/UI",
		});
	}
}
