// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieGenerationStructs.h"
#include "CraftingActionConfig.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemGenerationConfig.generated.h"

/**
 * Configurable item generation wrapper class.
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemGenerationConfig : public UFaerieCraftingActionConfig
{
	GENERATED_BODY()

	friend class UFaerieCraftingLibrary;

public:
	UFaerieItemGenerationConfig();

	//~ UObject
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	//~ UObject

	UFUNCTION(BlueprintCallable, Category = "Faerie|GenerationDriver")
	TInstancedStruct<FFaerieGeneratorAmountBase> GetAmountResolver() const;

	void Resolve(TArray<Faerie::FPendingItemGeneration>& Generations, USquirrel* Squirrel = nullptr) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Table", meta = (ShowOnlyInnerProperties))
	FFaerieWeightedPool DropPool;

	UPROPERTY(EditAnywhere, NoClear, Category = "Generator", meta = (ExcludeBaseStruct, DisplayName = "Procedure"))
	TInstancedStruct<FFaerieGenerationProcedureBase> ProcedureResolver;

	UPROPERTY(EditAnywhere, NoClear, Category = "Generator", meta = (ExcludeBaseStruct, DisplayName = "Amount"))
	TInstancedStruct<FFaerieGeneratorAmountBase> AmountResolver;
};