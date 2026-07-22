// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieHash.h"
#include "FaerieHashStatics.h"
#include "FaerieSlotTag.h"

class UFaerieEquipmentManager;
class UFaerieEquipmentHashAsset;

namespace Faerie::Hash
{
	FAERIEEQUIPMENT_API FFaerieHash HashEquipment(TNotNull<const UFaerieEquipmentManager*> Manager, const TSet<FFaerieSlotTag>& Slots, const FItemHashFunction& Function);

	FAERIEEQUIPMENT_API bool ExecuteHashInstructions(TNotNull<const UFaerieEquipmentManager*> Manager, TNotNull<const UFaerieEquipmentHashAsset*> Asset);
}