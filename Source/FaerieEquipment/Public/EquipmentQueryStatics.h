// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

struct FFaerieEquipmentSetQuery;
struct FFaerieItemProxy;
class UFaerieEquipmentManager;

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, FFaerieItemProxy& PassingSlot);
}