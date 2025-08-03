// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingLibrary.generated.h"

struct FFaerieItemCraftingSlots;
struct FFaerieItemSlotHandle;
struct FGeneratorAmountBase;
struct FWeightedDrop;
class IFaerieItemSlotInterface;
class UFaerieItemTemplate;
class UItemGenerationConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogCraftingLibrary, Log, All);

UCLASS()
class UCraftingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|Crafting")
	static UItemGenerationConfig* CreateGenerationDriver(const TArray<FWeightedDrop>& DropList, const FGeneratorAmountBase& Amount);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Item Creator")
	static void GetCraftingSlots(const TScriptInterface<IFaerieItemSlotInterface> Interface, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Item Creator", meta = (DisplayName = "Get Crafting Slots (Message)"))
	static void GetCraftingSlots_Message(UObject* Object, FFaerieItemCraftingSlots& Slots);

	UFUNCTION(BlueprintCallable, Category = "Item Creator")
	static bool IsSlotOptional(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name);

	UFUNCTION(BlueprintCallable, Category = "Item Creator")
	static bool FindSlot(const TScriptInterface<IFaerieItemSlotInterface> Interface, const FFaerieItemSlotHandle& Name, UFaerieItemTemplate*& OutSlot);
};