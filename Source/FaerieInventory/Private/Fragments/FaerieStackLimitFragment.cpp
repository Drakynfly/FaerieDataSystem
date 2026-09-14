// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieStackLimitFragment.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStackLimitFragment)

namespace Faerie::Container
{
	int32 GetItemStackLimit(const FMassEntityManager* EntityManager, const FFaerieItemInstance& Item)
	{
		// Enforce stack limit to 1, if item has potential to have variation between instances
		if (Item.IsMutable()) return 1;

		auto LimitView = ItemData::GetEntityFragmentOrDefault<FFaerieStackLimitFragment>(EntityManager, Item);

		// If the item has its own Limit Fragment, defer to member function.
		if (LimitView.IsValid())
		{
			// Interpret a limit of 0 as no limit.
			if (LimitView->MaxStackSize <= 0) return ItemData::UnlimitedStack;

			// Otherwise, use authored value for stack size.
			return LimitView->MaxStackSize;
		}

		// If no stack limiter is present, and the item is immutable, it can always stack
		return ItemData::UnlimitedStack;
	}
}