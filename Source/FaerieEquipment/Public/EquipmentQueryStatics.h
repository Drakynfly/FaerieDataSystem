// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

struct FFaerieEquipmentSetQuery;
class UFaerieEquipmentManager;
class UFaerieEquipmentSlot;

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot);
}