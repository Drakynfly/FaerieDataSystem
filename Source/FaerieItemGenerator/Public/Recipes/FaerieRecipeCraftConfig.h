// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemSlotInterface.h"
#include "FaerieRecipeCraftConfig.generated.h"

class UFaerieItemRecipe;

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieRecipeCraftConfig : public UDataAsset, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

	friend struct FFaerieCraftRecipeAction;

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	//~ IFaerieItemSlotInterface
	virtual FFaerieItemCraftingSlots GetCraftingSlots() const override;
	//~ IFaerieItemSlotInterface

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Config")
	TObjectPtr<UFaerieItemRecipe> Recipe;
};