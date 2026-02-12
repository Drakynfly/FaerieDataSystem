// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItem.h"
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

	FFaerieItemStackView() = default;

	FFaerieItemStackView(const TWeakObjectPtr<const UFaerieItem> ItemData, const int32 Copies)
	  : Item(ItemData),
		Copies(Copies) {}

	FFaerieItemStackView(const FFaerieItemStack& Stack)
	  : Item(Stack.Item),
		Copies(Stack.Copies) {}

	// The item being counted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	TWeakObjectPtr<const UFaerieItem> Item;

	// Copies in this stack
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FaerieItemStack")
	int32 Copies = 0;

	bool IsValid() const
	{
		return Item.IsValid() && Faerie::ItemData::IsValidStackAmount(Copies);
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemStackView& Other) const
	{
		return Item == Other.Item
			   && Copies == Other.Copies;
	}
};