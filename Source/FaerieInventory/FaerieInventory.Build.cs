﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieInventory : ModuleRules
{
    public FaerieInventory(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        // This module does not, and should not, have anything to do with FaerieEquipment

        // Engine dependencies
        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "NetCore",
                "StructUtils",
                "Slate",
                "UMG"
            });

        // Plugin dependencies
        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieItemData"
            });
    }
}