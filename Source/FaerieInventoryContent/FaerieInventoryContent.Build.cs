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
                "ModelViewViewModel",
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
                "FaerieItemMesh"
            });

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "FaerieDataUtils",
                "GeometryScriptingCore",
                "GeometryFramework",
                "MassEntity",
                "Squirrel"
            });

        SetupIrisSupport(Target);
    }
}