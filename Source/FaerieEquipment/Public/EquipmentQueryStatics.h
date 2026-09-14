// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

struct FFaerieEquipmentSetQuery;
class UFaerieEquipmentManager;
class UFaerieItemStackContainer;

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieItemStackContainer*& PassingSlot);
}