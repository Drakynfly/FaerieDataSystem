// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieAssetInfo.h"
#include "UObject/Object.h"
#include "FaerieItemSource.h"
#include "GenerationStructs.h"

#include "FaerieItemPool.generated.h"

USTRUCT()
struct FFaerieWeightedDropPool
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Table")
	TArray<FWeightedDrop> DropList;

	// Generates a drop from this pool, using the provided random weight, which must be a value between 0 and 1.
	const FTableDrop* GetDrop(double RanWeight) const;

#if WITH_EDITOR
	// Calculate the percentage each drop has to be chosen.
	void CalculatePercentages();

	// Keeps the table sorted by Weight.
	void SortTable();
#endif
};

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
	virtual UFaerieItem* CreateItemInstance(const UItemInstancingContext* Context) const override;
	//~ IFaerieItemSource

	const FTableDrop* GetDrop(double RanWeight) const;
	const FTableDrop* GetDrop_Seeded(USquirrel* Squirrel) const;

protected:
	// Generates a drop from this table, using the provided random weight, which must be a value between 0 and 1.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemPool")
	FTableDrop GenerateDrop(double RanWeight) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Faerie|ItemPool", DisplayName = "Generate Drop (Seeded)")
	FTableDrop GenerateDrop_Seeded(USquirrel* Squirrel) const;

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Table")
	FFaerieAssetInfo TableInfo;

	UPROPERTY(EditAnywhere, Category = "Table")
	FFaerieWeightedDropPool DropPool;

private:
	UPROPERTY(VisibleAnywhere)
	bool HasMutableDrops = false;
};