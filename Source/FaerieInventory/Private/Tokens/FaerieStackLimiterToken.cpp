﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieStackLimiterToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieStackLimiterToken)

/* static */ int32 UFaerieStackLimiterToken::GetItemStackLimit(const UFaerieItem* Item)
{
	if (!ensure(IsValid(Item)))
	{
		return 0;
	}

	// Enforce stack limit to 1, if item has potential to have variation between instances
	if (Item->IsDataMutable()) return 1;

	if (auto&& Limiter = Item->GetToken<UFaerieStackLimiterToken>())
	{
		return Limiter->GetStackLimit();
	}

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