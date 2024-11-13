﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieInventoryContent : ModuleRules
{
    public FaerieInventoryContent(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        // Engine dependencies
        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "NetCore",
                "Slate",
                "SlateCore",
                "UMG"
            });

        // Plugin dependencies
        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieEquipment",
                "FaerieInventory",
                "FaerieItemGenerator",
                "FaerieItemData",
                "FaerieItemMesh"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "FaerieDataUtils",
                "GeometryScriptingCore",
                "GeometryFramework",
                "Squirrel"
            });
    }
}