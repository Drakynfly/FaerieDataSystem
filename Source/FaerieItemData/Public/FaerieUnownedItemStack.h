// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Mass/EntityHandle.h"
#include "FaerieItemDataDefines.h"
#include "FaerieUnownedItemStack.generated.h"

/**
 * A stack of items unowned by any container.
 * The contained item is not replicated and only marked as UPROP to prevent GC. This struct is used to temporarily hold
 * onto item instances while asynchronous logic is running, or occasionally for passing free items through Blueprint.
 * Do not try to replicate this struct!
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieUnownedItemStack
{
	GENERATED_BODY()

	FFaerieUnownedItemStack() = default;

	FFaerieUnownedItemStack(const FMassEntityHandle Item, const int32 Copies)
	  : Item(Item),
		Copies(Copies) {}

	UPROPERTY()
	FMassEntityHandle Item;

	// Copies in this stack
	UPROPERTY()
	int32 Copies = 0;

	[[nodiscard]] UE_REWRITE bool IsValid() const
	{
		return Item.IsSet() && Faerie::ItemData::IsValidStackAmount(Copies);
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieUnownedItemStack& Other) const
	{
		return Item == Other.Item && Copies == Other.Copies;
	}
};