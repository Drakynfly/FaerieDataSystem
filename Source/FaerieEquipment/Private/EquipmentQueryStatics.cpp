// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentQueryStatics.h"
#include "EntityManagerHelpers.h"
#include "EquipmentQueryTypes.h"

#include "FaerieEquipmentManager.h"

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(const TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, FFaerieItemProxy& PassingSlot)
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Manager->GetWorld());
		for (const FFaerieSlotTag& QueryTag : SetQuery.TagSet.Tags)
		{
			if (UFaerieItemStackContainer* Slot = Manager->FindSlot(EntityManager, QueryTag, true))
			{
				if (Slot->IsFilled())
				{
					const FFaerieItemProxy SlotProxy(EntityManager.AsShared(), Slot);
					if (SetQuery.Query.Filter.Execute(SlotProxy) != SetQuery.Query.InvertFilter)
					{
						PassingSlot = SlotProxy;
						return true;
					}
				}
			}
		}

		PassingSlot = FFaerieItemProxy();
		return false;
	}
}