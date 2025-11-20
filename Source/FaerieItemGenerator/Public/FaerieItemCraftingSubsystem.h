// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "FaerieCraftingRunner.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemCraftingSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGenerationActionOnCompleteBinding, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemCraftingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	using FRequestResult = TDelegate<void(EGenerationActionResult Success, const TArray<FFaerieItemStack>&)>;

	UFaerieCraftingRunner* SubmitCraftingRequest(const FFaerieCraftingRequestBase& Request, const FRequestResult& Callback);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemGeneration")
	UFaerieCraftingRunner* SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingRequestBase> Request, const FGenerationActionOnCompleteBinding& Callback);

	UFaerieCraftingRunner* SubmitCraftingRequest(const TInstancedStruct<FFaerieCraftingRequestBase>& Request, FRequestResult Callback);

private:
	void OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult Result, FRequestResult Callback);

private:
	// The Actions currently running.
	UPROPERTY(Transient)
	TSet<TObjectPtr<UFaerieCraftingRunner>> ActiveActions;
};