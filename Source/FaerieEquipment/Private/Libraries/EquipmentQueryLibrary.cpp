// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentQueryLibrary.h"
#include "EquipmentQueryStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentQueryLibrary)

bool UFaerieEquipmentQueryLibrary::RunEquipmentQuery(UFaerieEquipmentManager* Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot)
{
	return Faerie::Equipment::RunEquipmentQuery(Manager, SetQuery, PassingSlot);
}