// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.generated.h"

class UFaerieItem;

namespace Faerie::ItemData
{
	inline constexpr int32 UnlimitedStack = -1;

	static bool IsValidStack(const int32 Value)
	{
		return Value > 0 || Value == UnlimitedStack;
	}
}

/**
 * A simple stack of items.
 * The Item pointer is non-const and assumed to not be owned by anyone.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemStack
{
	GENERATED_BODY()

	FFaerieItemStack() = default;

	FFaerieItemStack(const UFaerieItem* ItemData, const int32 Copies)
	  : Item(const_cast<UFaerieItem*>(ItemData)), // @todo const_cast for now, until Item is const.
		Copies(Copies) {}

	// The item being counted
	// @todo This *really* should be a const pointer, but that makes a lot of APIs annoying...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	TObjectPtr<UFaerieItem> Item;

	// Copies in this stack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	int32 Copies = 0;

	friend bool operator==(const FFaerieItemStack& Lhs, const FFaerieItemStack& Rhs)
	{
		return Lhs.Item == Rhs.Item
			   && Lhs.Copies == Rhs.Copies;
	}

	friend bool operator!=(const FFaerieItemStack& Lhs, const FFaerieItemStack& Rhs)
	{
		return !(Lhs == Rhs);
	}
};