// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRunner.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemUpgradeAction.generated.h"

class UFaerieItemUpgradeConfig;

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeAction : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

private:
	void Execute(UFaerieCraftingRunner* Runner) const;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FFaerieItemProxy ItemProxy;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TObjectPtr<UFaerieItemUpgradeConfig> Config = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FFaerieCraftingFilledSlots Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeActionBulkNoPayment : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

private:
	void Execute(UFaerieCraftingRunner* Runner) const;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TArray<FFaerieItemStack> UpgradeTargets;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TObjectPtr<UFaerieItemUpgradeConfig> Config = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieItemBulkUpgradeElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FFaerieItemProxy ItemProxy;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FFaerieCraftingFilledSlots Slots;
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeActionBulk : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

private:
	void Execute(UFaerieCraftingRunner* Runner) const;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TArray<FFaerieItemBulkUpgradeElement> UpgradeTargets;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TObjectPtr<UFaerieItemUpgradeConfig> Config = nullptr;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;
};
