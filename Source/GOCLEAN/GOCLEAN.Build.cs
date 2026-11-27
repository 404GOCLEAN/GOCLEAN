// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GOCLEAN : ModuleRules
{
    public GOCLEAN(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore", 
            "EnhancedInput", 
            "AIModule", 
            "GameplayTasks", 
            "NavigationSystem", 
            "DeveloperSettings",
            "LevelSequence",
            "MovieScene",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });

        // Personal Folder
        PublicIncludePaths.AddRange(new string[] {
            "GOCLEAN/Public"
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "VivoxCore", "ApplicationCore" });


        // add macro
        PublicDefinitions.Add("PROTOTYPE_2026_02=1");
    }
}