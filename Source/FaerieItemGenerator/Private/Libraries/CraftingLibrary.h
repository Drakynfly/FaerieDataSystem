// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingLibrary.generated.h"

struct FFaerieItemCraftingSlots;
struct FFaerieItemSlotHandle;
struct FFaerieGeneratorAmountBase;
struct FFaerieWeightedDrop;
class IFaerieItemSlotInterface;
class UFaerieItemTemplate;
class UFaerieItemGenerationConfig;

UCLASS()
class UFaerieCraftingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|Crafting")
	static UFaerieItemGenerationConfig* CreateGenerationDriver(const TArray<FFaerieWeightedDrop>& DropList, const FFaerieGeneratorAmountBase& Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Item Creator")
	static void GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Item Creator", meta = (DisplayName = "Get Crafting Slots (Message)"))
	static void GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, Category = "Item Creator")
	static bool IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name);

	UFUNCTION(BlueprintCallable, Category = "Item Creator")
	static bool FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name, UFaerieItemTemplate*& OutSlot);
};