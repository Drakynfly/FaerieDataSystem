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
	TArray<FFaerieItemCraftingCostElement> Slots;
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
	virtual const FFaerieItemCraftingSlots* GetCraftingSlots() const PURE_VIRTUAL(IFaerieItemSlotInterface::GetCraftingSlots, return nullptr; )
};

namespace Faerie::Generation
{
	FAERIEITEMGENERATOR_API bool ForEachCraftingSlot(const FFaerieItemCraftingSlots& Slots, const TFunctionRef<bool(const FFaerieItemCraftingCostElement& Slot)>& Predicate);

	template <bool LogFailure>
	bool ValidateFilledSlots(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots);

	// Remove items/uses from the entries in Slots used to fund this action.
	FAERIEITEMGENERATOR_API bool ConsumeSlotCosts(FMassEntityManager& EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& Slots);

	FAERIEITEMGENERATOR_API const FFaerieItemCraftingCostElement* FindSlot(const FFaerieItemCraftingSlots& Slots, const FFaerieItemSlotHandle& Name);

	FAERIEITEMGENERATOR_API bool IsSlotOptional(const FFaerieItemCraftingSlots& Slots, const FFaerieItemSlotHandle& Name);
}