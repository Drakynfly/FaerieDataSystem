// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "UObject/Interface.h"
#include "ItemSlotHandle.h"
#include "StructUtils/StructView.h"

#include "FaerieItemSlotInterface.generated.h"

class UFaerieItemTemplate;

USTRUCT(BlueprintType)
struct FFaerieItemCraftingSlots
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemCraftingSlots")
	TMap<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>> RequiredSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemCraftingSlots")
	TMap<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>> OptionalSlots;
};


USTRUCT(BlueprintType)
struct FFaerieCraftingFilledSlots
{
	GENERATED_BODY()

	// Crafting slots, and the proxy being used to provide data to them.
	UPROPERTY()
	TMap<FFaerieItemSlotHandle, FFaerieItemProxy> Slots;
};

UINTERFACE(MinimalAPI, Meta = (CannotImplementInterfaceInBlueprint))
class UFaerieItemSlotInterface : public UInterface
{
	GENERATED_BODY()
};

using FFaerieCraftingSlotsView = TConstStructView<FFaerieItemCraftingSlots>;

/**
 *
 */
class FAERIEITEMGENERATOR_API IFaerieItemSlotInterface
{
	GENERATED_BODY()

public:
	// Returns a struct view of type 'FFaerieItemCraftingSlots'
	virtual FFaerieCraftingSlotsView GetCraftingSlots() const PURE_VIRTUAL(IFaerieItemSlotInterface::GetCraftingSlots, return FFaerieCraftingSlotsView(); )
};

namespace Faerie::Generation
{
	FAERIEITEMGENERATOR_API bool ValidateFilledSlots(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots);

	// Remove items and durability from the entries in Slots used to fund this action.
	FAERIEITEMGENERATOR_API void ConsumeSlotCosts(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemCraftingSlots& CraftingSlots);

	FAERIEITEMGENERATOR_API FFaerieCraftingSlotsView GetCraftingSlots(const IFaerieItemSlotInterface* Interface);
	FAERIEITEMGENERATOR_API bool IsSlotOptional(const IFaerieItemSlotInterface* Interface, const FFaerieItemSlotHandle& Name);
	FAERIEITEMGENERATOR_API bool FindSlot(const IFaerieItemSlotInterface* Interface, const FFaerieItemSlotHandle& Name, UFaerieItemTemplate*& OutSlot);
}