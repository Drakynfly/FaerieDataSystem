// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieStackLimiterToken.h"
#include "FaerieItem.h"
#include "FaerieItemStack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStackLimiterToken)

/* static */ int32 UFaerieStackLimiterToken::GetItemStackLimit(const UFaerieItem* Item)
{
	if (!ensure(IsValid(Item)))
	{
		return 0;
	}

	// If the item has its own Limiter Token, defer to member function.
	if (auto&& Limiter = Item->GetToken<UFaerieStackLimiterToken>())
	{
		return Limiter->GetStackLimit();
	}

	// Enforce stack limit to 1, if item has potential to have variation between instances
	if (Item->CanMutate()) return 1;

	// If no stack limiter is present, and the item is immutable, it can always stack
	return Faerie::ItemData::UnlimitedStack;
}

int32 UFaerieStackLimiterToken::GetStackLimit() const
{
	// Enforce stack limit to 1, if item has potential to have variation between instances
	if (IsOuterItemMutable()) return 1;

	// Interpret a limit of 0 as no limit.
	if (MaxStackSize <= 0) return Faerie::ItemData::UnlimitedStack;

	// Otherwise, use authored value for stack size.
	return MaxStackSize;
}