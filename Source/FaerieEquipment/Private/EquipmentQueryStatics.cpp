// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentQueryStatics.h"
#include "EquipmentQueryTypes.h"

#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentSlot.h"

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(const TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot)
	{
		for (auto&& QueryTag : SetQuery.TagSet.Tags)
		{
			if (auto&& Slot = Manager->FindSlot(QueryTag, true))
			{
				if (Slot->IsFilled())
				{
					if (SetQuery.Query.Filter.Execute(Slot->View()) != SetQuery.Query.InvertFilter)
					{
						PassingSlot = Slot;
						return true;
					}
				}
			}
		}

		PassingSlot = nullptr;
		return false;
	}
}