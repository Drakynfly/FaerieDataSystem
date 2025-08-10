// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRequestSlot.h"
#include "FaerieCraftingRunner.h"
#include "FaerieRecipeCraftRequest.generated.h"

class UFaerieRecipeCraftConfig;

// The client assembles these via UI and submits them to the server for validation when requesting an item craft.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieRecipeCraftRequest : public FFaerieCraftingRequestBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

	// These should be sourced from cooked assets, or spawned by the server, if needed at runtime. Clients cannot create them.
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TObjectPtr<UFaerieRecipeCraftConfig> Config;

	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TArray<FFaerieCraftingRequestSlot> Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	bool RunConsumeStep = false;
};