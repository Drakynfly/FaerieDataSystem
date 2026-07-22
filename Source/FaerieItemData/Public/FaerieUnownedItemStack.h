// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
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

	FFaerieUnownedItemStack(const Faerie::ItemData::FReference& Reference, const int32 Copies)
	  : Instance(Reference.GetInstance()),
		Copies(Copies) {}

	// The item instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnownedItemStack")
	FFaerieItemInstance Instance;

	// Copies in this stack
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UnownedItemStack")
	int32 Copies = 0;

	[[nodiscard]] UE_REWRITE bool IsValid() const
	{
		return Instance.IsValid() && Faerie::ItemData::IsValidStackAmount(Copies);
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieUnownedItemStack& Other) const
	{
		return Instance == Other.Instance && Copies == Other.Copies;
	}
};