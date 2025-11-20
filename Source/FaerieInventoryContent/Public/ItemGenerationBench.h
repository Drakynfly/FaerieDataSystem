// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "BenchBehaviorBase.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemGenerationBench.generated.h"

struct FFaerieItemStack;
struct FFaerieItemMutator;
class USquirrel;
class UFaerieItemGenerationConfig;

/**
 *
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UItemGenerationBench : public UBenchBehaviorBase
{
	GENERATED_BODY()

public:
	UItemGenerationBench();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, NoClear, Category = "ItemGeneration", meta = (NoResetToDefault))
	TArray<TObjectPtr<UFaerieItemGenerationConfig>> Drivers;

	// Mutator to run on applicable items generated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemGeneration", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemMutator> Mutator;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, NoClear, Category = "ItemGeneration", meta = (NoResetToDefault, DisplayThumbnail = false, ShowInnerProperties))
	TObjectPtr<USquirrel> Squirrel;
};