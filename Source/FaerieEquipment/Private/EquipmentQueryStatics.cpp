// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentQueryStatics.h"
#include "EntityManagerHelpers.h"
#include "EquipmentQueryTypes.h"

#include "FaerieEquipmentManager.h"

namespace Faerie::Equipment
{
	bool RunEquipmentQuery(const TNotNull<UFaerieEquipmentManager*> Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieItemStackContainer*& PassingSlot)
	{
		const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Manager->GetWorld());
		for (auto&& QueryTag : SetQuery.TagSet.Tags)
		{
			if (auto&& Slot = Manager->FindSlot(EntityManager, QueryTag, true))
			{
				if (Slot->IsFilled())
				{
					if (SetQuery.Query.Filter.Execute(FFaerieItemProxy(Slot)) != SetQuery.Query.InvertFilter)
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