// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

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
                "GameplayTags",
                "ModelViewViewModel"
            }
        );

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieItemData",
                "FaerieInventory",
                "FaerieDataUtils"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "MassEntity",
                "NetCore",
                "Squirrel",
            }
        );
    }
}