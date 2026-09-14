// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class FaerieDataSystemEditor : ModuleRules
{
    public FaerieDataSystemEditor(ReadOnlyTargetRules Target) : base (Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore"
            });

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "FaerieDataUtils"
            });

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "AssetDefinition",
                "SceneOutliner",
                "Slate",
                "SlateCore",
                "InputCore",
                "KismetWidgets",
                "Kismet",  // for FWorkflowCentricApplication
                "KismetCompiler",
                "ToolMenus",
                "PropertyEditor",
                "RenderCore",
                "ContentBrowser",
                "BlueprintGraph",
                "WorkspaceMenuStructure",
                "EditorStyle",
                "EditorWidgets",
                "Projects",
                "AssetRegistry",
                "ClassViewer",
                "GraphEditor",
                "AssetTools",
                "GameplayTags",
                "GameplayTagsEditor",
                "UnrealEd",
            });

        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(GetModuleDirectory("PropertyEditor"), "Private")
            }
        );
    }
}