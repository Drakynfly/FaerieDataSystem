// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "GenerationStructs.h"
#include "CraftingActionConfig.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemPool.h"
#include "ItemGeneratorConfig.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogItemGenConfig, Log, All);

// A struct containing information about a generated item drop that has not yet been parsed into entries.
USTRUCT(BlueprintType)
struct FPendingItemGeneration
{
	GENERATED_BODY()

	// The drop that was generated.
	UPROPERTY()
	FTableDrop Drop;

	// Initial amount of items dropped. May be split later into multiple entries, if this number is larger than stack
	// limit on the asset.
	UPROPERTY()
	int32 Count = 0;

	// Squirrel pointer given to the Pending Generation. Required when using random generation.
	UPROPERTY()
	TObjectPtr<USquirrel> Squirrel = nullptr;

	bool IsValid() const
	{
		return Drop.IsValid() && Count > 0;
	}
};

/**
 * Configurable item generation wrapper class.
 */
UCLASS()
class FAERIEITEMGENERATOR_API UItemGenerationConfig : public UCraftingActionConfig
{
	GENERATED_BODY()

	friend class UCraftingLibrary;

public:
	UItemGenerationConfig();

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Faerie|GenerationDriver")
	FGeneratorAmountBase GetAmountResolver() const;

	void Resolve(TArray<FPendingItemGeneration>& Generations) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Table", meta = (ShowOnlyInnerProperties))
	FFaerieWeightedDropPool DropPool;

	UPROPERTY(EditAnywhere, NoClear, Category = "Generator", meta = (ExcludeBaseStruct, DisplayName = "Amount"))
	TInstancedStruct<FGeneratorAmountBase> AmountResolver;
};