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
                "UMGEditor",
                "UMGWidgetPreview"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "AdvancedPreviewScene",
                "AssetDefinition",
                "BlueprintGraph",
                "CoreUObject",
                "Engine",
                "FaerieItemCard",
                "FaerieItemMesh",
                "GameplayTagsEditor",
                "KismetCompiler",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "FaerieInventoryContent" // Refactor modules...
            }
        );
    }
}