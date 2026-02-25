// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieCraftRecipeAction.generated.h"

class UFaerieRecipeCraftConfig;

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieCraftRecipeAction : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

	// These should be sourced from cooked assets, or spawned by the server, if needed at runtime. Clients cannot create them.
	UPROPERTY(BlueprintReadWrite, Category = "Craft Recipe Action")
	TObjectPtr<UFaerieRecipeCraftConfig> Config;

	UPROPERTY(BlueprintReadWrite, Category = "Craft Recipe Action")
	FFaerieCraftingFilledSlots Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY(BlueprintReadWrite, Category = "Craft Recipe Action")
	bool RunConsumeStep = false;
};