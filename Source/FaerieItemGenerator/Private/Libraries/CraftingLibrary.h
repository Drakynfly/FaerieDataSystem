// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingLibrary.generated.h"

struct FFaerieCraftingFilledSlots;
struct FFaerieItemCraftingCostElement;
struct FFaerieItemCraftingSlots;
struct FFaerieItemSlotHandle;
class IFaerieItemSlotInterface;

UCLASS()
class UFaerieCraftingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary")
	static void GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary", meta = (DisplayName = "Get Crafting Slots (Message)"))
	static void GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary")
	static bool TestCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieCraftingFilledSlots& FilledSlots);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingLibrary")
	static bool IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingLibrary")
	static bool FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name, FFaerieItemCraftingCostElement& OutSlot);
};