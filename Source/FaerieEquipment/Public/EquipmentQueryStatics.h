// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

struct FFaerieEquipmentSetQuery;
class UFaerieEquipmentManager;
class UFaerieEquipmentSlot;

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot);
}