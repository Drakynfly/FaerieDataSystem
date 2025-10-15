// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieAssetInfo.h"
#include "UObject/Object.h"
#include "FaerieItemSource.h"
#include "Generation/FaerieGenerationStructs.h"

#include "FaerieItemPool.generated.h"

class USquirrel;

/**
 * A Faerie Item Pool is a list of possible item generations, each with a weight that determined its frequency.
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemPool : public UObject, public IFaerieItemSource
{
	GENERATED_BODY()

public:
	UFaerieItemPool();

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

public:
	//~ IFaerieItemSource
	virtual bool CanBeMutable() const override;
	virtual FFaerieAssetInfo GetSourceInfo() const override;
	virtual TOptional<FFaerieItemStack> CreateItemStack(const FFaerieItemInstancingContext* Context) const override;
	//~ IFaerieItemSource

	const FFaerieTableDrop* GetDrop(double RanWeight) const;
	const FFaerieTableDrop* GetDrop_Seeded(USquirrel* Squirrel) const;

	TConstArrayView<FFaerieWeightedDrop> ViewDropPool() const;

protected:
	// Generates a drop from this table, using the provided random weight, which must be a value between 0 and 1.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemPool")
	FFaerieTableDrop GenerateDrop(double RanWeight) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemPool", DisplayName = "Generate Drop (Seeded)")
	FFaerieTableDrop GenerateDrop_Seeded(USquirrel* Squirrel) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Table")
	FFaerieAssetInfo TableInfo;

	UPROPERTY(EditAnywhere, Category = "Table")
	FFaerieWeightedPool DropPool;

private:
	UPROPERTY(VisibleAnywhere)
	bool HasMutableDrops = false;
};