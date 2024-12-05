﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieItemGenerator : ModuleRules
{
    public FaerieItemGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "GameplayTags"
            }
        );

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieItemData",
                "FaerieDataUtils"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "Squirrel"
            }
        );
    }
}