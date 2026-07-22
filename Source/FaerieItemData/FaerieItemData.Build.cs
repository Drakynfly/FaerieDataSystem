// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieItemData : ModuleRules
{
    public FaerieItemData(ReadOnlyTargetRules Target) : base(Target)
    {
        FaerieDataUtils.ApplySharedModuleSetup(this, Target);
        SetupIrisSupport(Target);

        PublicDependencyModuleNames.AddRange(
            new []
            {
                "Core",
                "DeveloperSettings",
                "GameplayTags",
                "FaerieDataUtils",
                "MassCore",
                "MassEntity",
                "MassSpawner", // For UMassEntityConfigAsset
                "ModelViewViewModel",
                "NetCore",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new []
            {
                "CoreUObject",
                "Engine",
                "Squirrel"
            }
        );

		if (Target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.Add("UnrealEd"); // For USceneThumbnailInfo in FaerieItemAsset.h
		}
    }
}