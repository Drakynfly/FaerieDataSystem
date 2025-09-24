// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

using UnrealBuildTool;

public class FaerieEquipment : ModuleRules
{
	public FaerieEquipment(ReadOnlyTargetRules Target) : base(Target)
	{
		FaerieDataUtils.ApplySharedModuleSetup(this, Target);

		PublicDependencyModuleNames.AddRange(
			new []
			{
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",
				"GameplayTags"
			});

		PublicDependencyModuleNames.AddRange(
			new []
			{
				"FaerieItemData",
				"FaerieItemMesh",
				"FaerieInventory"
			});

		PrivateDependencyModuleNames.AddRange(
			new []
			{
				"FaerieDataUtils",
				"Squirrel"
			});
	}
}