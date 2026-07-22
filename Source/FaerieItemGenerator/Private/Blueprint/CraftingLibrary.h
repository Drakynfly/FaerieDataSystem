// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingLibrary.generated.h"

struct FFaerieCraftingFilledSlots;
struct FFaerieItemCraftingCostElement;
struct FFaerieItemCraftingSlots;
struct FFaerieItemSlotHandle;
class IFaerieItemSlotInterface;

UCLASS()
class UFaerieGenerationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary")
	static void GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary", meta = (DisplayName = "Get Crafting Slots (Message)"))
	static void GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|CraftingLibrary", meta = (WorldContext = "WorldContextObj", DefaultToSelf = "WorldContextObj"))
	static bool TestCraftingSlots(const UObject* WorldContextObj, const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieCraftingFilledSlots& FilledSlots);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingLibrary", meta = (WorldContext = "WorldContextObj", DefaultToSelf = "WorldContextObj"))
	static bool ConsumeSlotCosts(const UObject* WorldContextObj, const FFaerieCraftingFilledSlots& FilledSlots, const TScriptInterface<IFaerieItemSlotInterface>& CraftingSlots);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingLibrary")
	static bool IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingLibrary")
	static bool FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name, FFaerieItemCraftingCostElement& OutSlot);

	/*
	 * Verify that an item can be "consumed" by the passed in Consumer Actor, with a given Cost value.
	 * Cost can represent the "charged" or "uses" consumed by a single action. Defaults to 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "Consumable")
	static bool CanConsume(const FFaerieItemProxy& Proxy, UScriptStruct* ConsumableType, const AActor* Consumer, int32 Cost = 1);

	UFUNCTION(BlueprintCallable, Category = "Consumable")
	static bool TryConsume(const FFaerieItemProxy& Proxy, UScriptStruct* ConsumableType, AActor* Consumer, int32 Cost = 1);
};