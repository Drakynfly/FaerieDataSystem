// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieItemMesh : ModuleRules
{
    public FaerieItemMesh(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
				"DeveloperSettings",
                "GameplayTags"
            });

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieItemData"
            });

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "SkeletalMerging"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "GeometryCore",
                "GeometryScriptingCore",
                "GeometryFramework"
            });
    }
}