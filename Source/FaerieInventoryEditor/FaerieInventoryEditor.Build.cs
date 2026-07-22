// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieInventoryEditor : ModuleRules
{
    public FaerieInventoryEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "FaerieInventory",
                "FaerieDataSystemEditor"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "GameplayTagsEditor",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "UnrealEd"
            }
        );
    }
}