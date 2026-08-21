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
	virtual void GetRequiredAssets(TAdderRef<FSoftObjectPath> Array);

	virtual bool CanApplyUpgrade(const FMassEntityManager* EntityManager, const FFaerieItemProxy& Proxy) const { return true; }

	virtual bool CanPayCost(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const { return true; }

	virtual void PayCost(FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const {}

	virtual bool ApplyUpgrade(FMassEntityManager* EntityManager, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
		PURE_VIRTUAL(UFaerieItemUpgradeConfigBase::ApplyUpgrade, return false; )
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

	virtual void GetRequiredAssets(TAdderRef<FSoftObjectPath> Array) override;

	virtual bool CanPayCost(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;

	virtual void PayCost(FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;

	virtual bool ApplyUpgrade(FMassEntityManager* EntityManager, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

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
	virtual bool CanApplyUpgrade(const FMassEntityManager* EntityManager, const FFaerieItemProxy& Proxy) const override;
	virtual bool CanPayCost(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;
	virtual void PayCost(FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const override;
	virtual bool ApplyUpgrade(FMassEntityManager* EntityManager, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Can Apply Upgrade"))
	bool BP_CanApplyUpgrade(const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Can Pay Cost"))
	bool BP_CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Pay Cost"))
	void BP_PayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade Config", meta = (DisplayName = "Apply Upgrade"))
	bool BP_ApplyUpgrade(const FFaerieItemProxy& Proxy, USquirrel* Squirrel) const;
};