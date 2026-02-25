// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemAsset.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemRecipe.generated.h"

/**
 * A wrapper around an item source that requires being fed crafting data to generate an item instance
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemRecipe : public UObject, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

public:
	//~ IFaerieItemSlotInterface
	virtual FFaerieItemCraftingSlots GetCraftingSlots() const override;
	//~ IFaerieItemSlotInterface

	TScriptInterface<IFaerieItemSource> GetItemSource() const { return ItemSource; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faerie|ItemRecipe")
	TScriptInterface<IFaerieItemSource> ItemSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faerie|ItemRecipe")
	FFaerieItemCraftingSlots CraftingSlots;
};