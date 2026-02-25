// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "FaerieItemSlotInterface.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemUpgradeConfig.generated.h"

struct FFaerieCraftingActionData;
struct FFaerieItemMutator;

/**
 *
 */
UCLASS(Abstract, Const)
class FAERIEITEMGENERATOR_API UFaerieItemUpgradeConfigBase : public UObject, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

protected:
	//~ IFaerieItemSlotInterface
	//virtual FFaerieItemCraftingSlots GetCraftingSlots() const override;
	//~ IFaerieItemSlotInterface

public:
	void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array);

	virtual bool CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const { return true; }

	virtual void PayCost(const FFaerieCraftingFilledSlots& FilledSlots, FFaerieItemStackView View) const {}

	virtual bool ApplyUpgrade(FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
		PURE_VIRTUAL(UFaerieItemUpgradeConfigBase::ApplyUpgrade, return false; )

	virtual bool ApplyPayment(FFaerieCraftingActionData& Stacks) const
		PURE_VIRTUAL(UFaerieItemUpgradeConfigBase::ApplyPayment, return false; )

	// Should this upgrade release proxies from their owner, rather than mutate in place.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Config")
	bool ReleaseWhileOperating = false;
};

class UFaerieItemUpgradeConfig;

USTRUCT()
struct FFaerieItemMutatorContext_UpgradeConfig : public FFaerieItemMutatorContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UFaerieItemUpgradeConfig> Config;
};

/**
 *
 */
UCLASS(Abstract)
class FAERIEITEMGENERATOR_API UFaerieItemUpgradeConfig : public UFaerieItemUpgradeConfigBase
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	//~ IFaerieItemSlotInterface
	virtual FFaerieItemCraftingSlots GetCraftingSlots() const override;
	//~ IFaerieItemSlotInterface

public:
	virtual bool CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const override;

	virtual void PayCost(const FFaerieCraftingFilledSlots& FilledSlots, FFaerieItemStackView View) const override;

	// Upgrade configs have crafting slots dependent on the item being upgraded.
	virtual FFaerieItemCraftingSlots GetCraftingSlots(FFaerieItemStackView View) const;

	virtual bool ApplyUpgrade(FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

	// Mutator struct, created inline.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Config", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemMutator> Mutator;

	// Should this upgrade fail if the mutator cannot apply?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Config")
	bool RequireMutatorToRun = false;
};

/**
 *
 */
UCLASS(Abstract, Blueprintable)
class FAERIEITEMGENERATOR_API UFaerieItemUpgradeConfig_BlueprintBase final : public UFaerieItemUpgradeConfigBase
{
	GENERATED_BODY()

protected:
	//~ IFaerieItemSlotInterface
	virtual FFaerieItemCraftingSlots GetCraftingSlots() const override { return {}; }
	//~ IFaerieItemSlotInterface

public:
	virtual bool CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const override;
	virtual void PayCost(const FFaerieCraftingFilledSlots& FilledSlots, FFaerieItemStackView View) const override;
	virtual bool ApplyUpgrade(FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Can Pay Cost"))
	bool BP_CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Pay Cost"))
	void BP_PayCost(const FFaerieCraftingFilledSlots& FilledSlots, FFaerieItemStackView View) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Apply Upgrade"))
	bool BP_ApplyUpgrade(UPARAM(ref) FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const;
};