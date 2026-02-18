// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRunner.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieCraftRecipeAction.generated.h"

class UFaerieRecipeCraftConfig;

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieCraftRecipeAction : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

	// These should be sourced from cooked assets, or spawned by the server, if needed at runtime. Clients cannot create them.
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TObjectPtr<UFaerieRecipeCraftConfig> Config;

	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	FFaerieCraftingFilledSlots Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	bool RunConsumeStep = false;
};