// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentHashLibrary.h"
#include "EquipmentHashStatics.h"
#include "DelegateCommon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentHashLibrary)

FFaerieHash UFaerieEquipmentHashLibrary::HashEquipment(const UFaerieEquipmentManager* Manager,
																const FFaerieEquipmentHashConfig& Config)
{
	if (!Config.HashFunction.IsBound())
	{
		return FFaerieHash();
	}

	return Faerie::Hash::HashEquipment(Manager, Config.Slots, DYNAMIC_TO_LAMBDA(Config.HashFunction));
}

bool UFaerieEquipmentHashLibrary::ExecuteHashInstructions(const UFaerieEquipmentManager* Manager, const UFaerieEquipmentHashAsset* Asset)
{
	return Faerie::Hash::ExecuteHashInstructions(Manager, Asset);
}

FBlueprintEquipmentHash UFaerieEquipmentHashLibrary::GetEquipmentHash_ByName()
{
	return AUTO_DELEGATE_STATIC(FBlueprintEquipmentHash, ThisClass, ExecHashItemByName);
}

int32 UFaerieEquipmentHashLibrary::ExecHashItemByName(const UFaerieItem* Item)
{
	if (Item)
	{
		return Faerie::Hash::HashItemByName(Item);
	}
	return 0;
}