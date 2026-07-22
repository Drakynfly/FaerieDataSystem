// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "FaerieItemSlotInterface.h"

#include "Mutators/ItemMutatorGroup.h"

#include "FaerieItemUpgradeConfig.generated.h"

struct FFaerieCraftingActionData;

/**
 *
 */
UCLASS(Abstract, Const)
class FAERIEITEMGENERATOR_API UFaerieItemUpgradeConfigBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array);

	virtual bool CanApplyUpgrade(TNotNull<UObject*> WorldContext, const FFaerieItemProxy& Proxy) const { return true; }

	virtual bool CanPayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const { return true; }

	virtual void PayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const {}

	virtual bool ApplyUpgrade(TNotNull<UObject*> WorldContext, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
		PURE_VIRTUAL(UFaerieItemUpgradeConfigBase::ApplyUpgrade, return false; )

	virtual bool ApplyPayment(TNotNull<UObject*> WorldContext, FFaerieCraftingActionData& Stacks) const
		PURE_VIRTUAL(UFaerieItemUpgradeConfigBase::ApplyPayment, return false; )
};

class UFaerieItemUpgradeConfig;

USTRUCT()
struct FFaerieItemMutatorContext_UpgradeConfig : public FFaerieItemMutatorContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UFaerieItemUpgradeConfig> Config;

	UE_REWRITE virtual const UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}
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

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array) override;

	virtual bool CanPayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;

	virtual void PayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;

	virtual bool ApplyUpgrade(TNotNull<UObject*> WorldContext, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

	// Mutators groups.
	UPROPERTY(EditAnywhere, Category = "Upgrade Config")
	FFaerieItemMutatorGroup Mutators;

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

public:
	virtual bool CanApplyUpgrade(TNotNull<UObject*> WorldContext, const FFaerieItemProxy& Proxy) const override;
	virtual bool CanPayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;
	virtual void PayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;
	virtual bool ApplyUpgrade(TNotNull<UObject*> WorldContext, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (WorldContext = "WorldContext", DisplayName = "Can Apply Upgrade"))
	bool BP_CanApplyUpgrade(UObject* WorldContext, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (WorldContext = "WorldContext", DisplayName = "Can Pay Cost"))
	bool BP_CanPayCost(UObject* WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (WorldContext = "WorldContext", DisplayName = "Pay Cost"))
	void BP_PayCost(UObject* WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (WorldContext = "WorldContext", DisplayName = "Apply Upgrade"))
	bool BP_ApplyUpgrade(UObject* WorldContext, UPARAM(ref) FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const;
};