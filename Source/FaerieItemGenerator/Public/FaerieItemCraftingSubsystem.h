// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "FaerieCraftingRunner.h"
#include "Recipes/FaerieRecipeCraftRequest.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemCraftingSubsystem.generated.h"

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

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemGeneration")
	UFaerieCraftingRunner* SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingRequestBase> Request);

private:
	void OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult Success);

	void SubmitCraftingRequest_Impl(const FFaerieRecipeCraftRequest& Request, bool Preview);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemGeneration")
	void PreviewCraftingRequest(const FFaerieRecipeCraftRequest& Request);

private:
	// The Actions currently running.
	UPROPERTY(Transient)
	TSet<TObjectPtr<UFaerieCraftingRunner>> ActiveActions;
};