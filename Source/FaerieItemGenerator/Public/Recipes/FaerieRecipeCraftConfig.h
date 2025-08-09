// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CraftingActionConfig.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieRecipeCraftConfig.generated.h"

class UFaerieItemRecipe;

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieRecipeCraftConfig : public UFaerieCraftingActionConfig, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

	friend struct FFaerieRecipeCraftRequest;

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual FFaerieCraftingSlotsView GetCraftingSlots() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Config")
	TObjectPtr<UFaerieItemRecipe> Recipe;
};