﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItem.h"
#include "FaerieItemProxy.h"
#include "FaerieItemStack.h"
#include "FaerieItemStackView.generated.h"

/**
 * A simple stack of items.
 * Item pointer is both const and weak, as it's assumed to be owned elsewhere.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemStackView
{
	GENERATED_BODY()

	FFaerieItemStackView() {}

	FFaerieItemStackView(const UFaerieItem* ItemData, const int32 Copies)
	  : Item(ItemData),
		Copies(Copies) {}

	FFaerieItemStackView(const FFaerieItemProxy Proxy)
	  : Item(Proxy.GetItemObject()),
		Copies(Proxy.GetCopies()) {}

	FFaerieItemStackView(const FFaerieItemStack& Stack)
	  : Item(Stack.Item),
		Copies(Stack.Copies) {}

	// The item being counted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	TWeakObjectPtr<const UFaerieItem> Item;

	// Copies in this stack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	int32 Copies = 0;

	// Changes the number of copies this stack represents.
	FFaerieItemStackView& Resize(const int32 Amount)
	{
		Copies = Amount;
		return *this;
	}

	friend bool operator==(const FFaerieItemStackView Lhs, const FFaerieItemStackView Rhs)
	{
		return Lhs.Item == Rhs.Item
			   && Lhs.Copies == Rhs.Copies;
	}

	friend bool operator!=(const FFaerieItemStackView Lhs, const FFaerieItemStackView Rhs)
	{
		return !(Lhs == Rhs);
	}
};