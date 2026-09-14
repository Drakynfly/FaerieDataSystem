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
				"GameplayTags",
				"MassCore",
				"MassEntity",
				"NetCore",
			});

		PublicDependencyModuleNames.AddRange(
			new []
			{
				"FaerieItemData",
				"FaerieItemMesh",
				"FaerieInventory",
				"FaerieInventoryContent"
			});

		PrivateDependencyModuleNames.AddRange(
			new []
			{
				"FaerieDataUtils",
				"Squirrel",
			});
	}
}