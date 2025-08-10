// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRequestSlot.h"
#include "FaerieCraftingRunner.h"
#include "FaerieItemUpgradeRequest.generated.h"

class UFaerieItemUpgradeConfig;

// The client assembles these via UI and submits them to the server for validation when requesting an item upgrade.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeRequest : public FFaerieCraftingRequestBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

private:
	void Execute(UFaerieCraftingRunner* Runner) const;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TScriptInterface<IFaerieItemDataProxy> ItemProxy;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TObjectPtr<UFaerieItemUpgradeConfig> Config = nullptr;

	// @todo use array instead of single
	//TArray<TObjectPtr<UItemUpgradeConfig> Configs;

	// Note: Acts like map, but must be array for replication.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TArray<FFaerieCraftingRequestSlot> Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;
};