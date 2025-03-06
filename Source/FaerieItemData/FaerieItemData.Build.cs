// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieItemData : ModuleRules
{
    public FaerieItemData(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "GameplayTags",
                "FaerieDataUtils",
                "NetCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "NetCore",
                "Squirrel",
                "Flakes",
                "FlakesJson"
            }
        );
    }
}