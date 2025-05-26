// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

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
                "FaerieInventory",
                "FaerieItemGenerator",
                "FaerieItemData",
                "FaerieItemMesh",
                "FaerieEquipment" // Again, please remove
            });

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "FaerieDataUtils",
                "GeometryScriptingCore",
                "GeometryFramework",
                "Squirrel"
            });

        SetupIrisSupport(Target);
    }
}