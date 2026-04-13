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
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"MotionWarping"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"UnrealCombat",
			"UnrealCombat/Variant_Platforming",
			"UnrealCombat/Variant_Platforming/Animation",
			"UnrealCombat/Variant_Combat",
			"UnrealCombat/Variant_Combat/AI",
			"UnrealCombat/Variant_Combat/Animation",
			"UnrealCombat/Variant_Combat/Gameplay",
			"UnrealCombat/Variant_Combat/Interfaces",
			"UnrealCombat/Variant_Combat/UI",
			"UnrealCombat/Variant_SideScrolling",
			"UnrealCombat/Variant_SideScrolling/AI",
			"UnrealCombat/Variant_SideScrolling/Gameplay",
			"UnrealCombat/Variant_SideScrolling/Interfaces",
			"UnrealCombat/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
