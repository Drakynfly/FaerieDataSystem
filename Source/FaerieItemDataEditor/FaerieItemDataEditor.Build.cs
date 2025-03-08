// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieItemDataEditor : ModuleRules
{
    public FaerieItemDataEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "FaerieItemData",
                "FaerieDataSystemEditor",
                "GameplayTags",
                "InputCore",
                "UMG",
                "UMGWidgetPreview"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "AdvancedPreviewScene",
                "AssetDefinition",
                "CoreUObject",
                "Engine",
                "FaerieItemCard",
                "FaerieItemMesh",
                "Slate",
                "SlateCore",
                "UnrealEd"
            }
        );
    }
}