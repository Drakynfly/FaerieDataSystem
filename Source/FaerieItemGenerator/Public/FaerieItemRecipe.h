// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemSlotInterface.h"
#include "FaerieItemRecipe.generated.h"

class IFaerieItemSource;

/**
 * A wrapper around an item source that requires being fed crafting data to generate an item instance
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemRecipe : public UObject, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

public:
	//~ IFaerieItemSlotInterface
	UE_REWRITE virtual const FFaerieItemCraftingSlots* GetCraftingSlots() const override { return &CraftingSlots; }
	//~ IFaerieItemSlotInterface

	UE_REWRITE TScriptInterface<IFaerieItemSource> GetItemSource() const { return ItemSource; }

protected:
	// @todo replace with FFaerieItemSourceObject
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faerie|ItemRecipe")
	TScriptInterface<IFaerieItemSource> ItemSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faerie|ItemRecipe")
	FFaerieItemCraftingSlots CraftingSlots;
};