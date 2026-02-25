// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "BenchBehaviorBase.h"
#include "ItemGenerationBench.generated.h"

class UFaerieItemUpgradeConfigBase;
class USquirrel;

/**
 *
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UItemGenerationBench : public UBenchBehaviorBase
{
	GENERATED_BODY()

public:
	UItemGenerationBench();

protected:
	// Upgrade to run on applicable items generated.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ItemGeneration", meta = (AllowAbstract = false))
	TSubclassOf<UFaerieItemUpgradeConfigBase> Config = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, NoClear, Category = "ItemGeneration", meta = (NoResetToDefault, DisplayThumbnail = false, ShowInnerProperties))
	TObjectPtr<USquirrel> Squirrel;
};