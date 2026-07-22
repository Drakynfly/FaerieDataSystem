// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieStackLimitFragment.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStackLimitFragment)

namespace Faerie::Container
{
	int32 GetItemStackLimit(const ItemData::FOptionalEntityManager& EntityManager, const ItemData::FReference& Item)
	{
		// Enforce stack limit to 1, if item has potential to have variation between instances
		if (Item->IsMutable()) return 1;

		auto LimitView = ItemData::GetEntityFragmentOrDefault<FFaerieStackLimitFragment>(EntityManager, Item);

		// If the item has its own Limit Fragment, defer to member function.
		if (LimitView.IsValid())
		{
			return LimitView->GetStackLimit();
		}

		// If no stack limiter is present, and the item is immutable, it can always stack
		return ItemData::UnlimitedStack;
	}
}

int32 FFaerieStackLimitFragment::GetStackLimit() const
{
	// Interpret a limit of 0 as no limit.
	if (MaxStackSize <= 0) return Faerie::ItemData::UnlimitedStack;

	// Otherwise, use authored value for stack size.
	return MaxStackSize;
}