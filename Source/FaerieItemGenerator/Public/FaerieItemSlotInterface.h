// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "UObject/Interface.h"
#include "ItemSlotHandle.h"

#include "FaerieItemSlotInterface.generated.h"

class UFaerieItemTemplate;

// A description of a cost that must be paid to use a crafting action.
USTRUCT(BlueprintType)
struct FFaerieItemCraftingCostElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ItemCraftingCostElement")
	FFaerieItemSlotHandle Name;

	// The item used for payment of this cost much match this template.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ItemCraftingCostElement")
	TObjectPtr<UFaerieItemTemplate> Template;

	// The quantity of payment required
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ItemCraftingCostElement")
	int32 Amount = 1;

	// If consumable uses can be consumed to pay for the cost, instead of stack copies.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ItemCraftingCostElement")
	bool PayInConsumableUses = true;

	// This slot does not have to be filled.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "ItemCraftingCostElement")
	bool Optional = false;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemSlotHandle& Handle) const
	{
		return Name == Handle;
	}
};

USTRUCT(BlueprintType)
struct FFaerieItemCraftingSlots
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemCraftingSlots")
	TArray<FFaerieItemCraftingCostElement> RequiredSlots;
};


USTRUCT(BlueprintType)
struct FFaerieCraftingFilledSlots
{
	GENERATED_BODY()

	// Crafting slots, and the proxy being used to provide data to them.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CraftingFilledSlots")
	TMap<FFaerieItemSlotHandle, FFaerieItemProxy> Slots;
};

UINTERFACE(MinimalAPI, Meta = (CannotImplementInterfaceInBlueprint))
class UFaerieItemSlotInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class FAERIEITEMGENERATOR_API IFaerieItemSlotInterface
{
	GENERATED_BODY()

public:
	// @todo deprecate this in favor of more fine grain functions so we aren't allocating/copying arrays wildly
	virtual FFaerieItemCraftingSlots GetCraftingSlots() const PURE_VIRTUAL(IFaerieItemSlotInterface::GetCraftingSlots, return FFaerieItemCraftingSlots(); )
};

namespace Faerie::Generation
{
	FAERIEITEMGENERATOR_API bool ValidateFilledSlots(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots);

	// Remove items and durability from the entries in Slots used to fund this action.
	FAERIEITEMGENERATOR_API bool ConsumeSlotCosts(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots);

	FAERIEITEMGENERATOR_API const FFaerieItemCraftingCostElement* FindSlot(TNotNull<const IFaerieItemSlotInterface*> Interface, const FFaerieItemSlotHandle& Name);

	FAERIEITEMGENERATOR_API bool IsSlotOptional(TNotNull<const IFaerieItemSlotInterface*> Interface, const FFaerieItemSlotHandle& Name);
}