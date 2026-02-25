// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemUpgradeAction.generated.h"

class UFaerieItemUpgradeConfigBase;

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeAction : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

private:
	void Execute(TNotNull<UFaerieItemCraftingRunner*> Runner);

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action")
	FFaerieItemProxy ItemProxy;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action")
	TObjectPtr<UFaerieItemUpgradeConfigBase> Config = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action")
	FFaerieCraftingFilledSlots Slots;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeActionBulkNoPayment : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

private:
	void Execute(TNotNull<UFaerieItemCraftingRunner*> Runner);

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action Bulk No Payment")
	TArray<FFaerieItemStack> UpgradeTargets;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action Bulk No Payment")
	TObjectPtr<UFaerieItemUpgradeConfigBase> Config = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieItemBulkUpgradeElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Element")
	FFaerieItemProxy ItemProxy;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Element")
	FFaerieCraftingFilledSlots Slots;
};

USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemUpgradeActionBulk : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

private:
	void Execute(TNotNull<UFaerieItemCraftingRunner*> Runner);

public:
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action Bulk")
	TArray<FFaerieItemBulkUpgradeElement> UpgradeTargets;

	// These should be safe to replicate, since they are (always?) sourced from cooked assets.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Action Bulk")
	TObjectPtr<UFaerieItemUpgradeConfigBase> Config = nullptr;

	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;
};
