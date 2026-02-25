// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieSubmitCraftingActionAsync.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFaerieAsyncCraftingActionCompleted, EGenerationActionResult, Result,
											 const FFaerieCraftingActionData&, Items);

/**
 * 
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieSubmitCraftingActionAsync : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|Crafting", DisplayName = "Submit Crafting Action Async",
		meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObj"))
	static UFaerieSubmitCraftingActionAsync* SubmitCraftingActionAsync(UObject* WorldContextObj,
		TInstancedStruct<FFaerieCraftingActionBase> Request);

	virtual void Activate() override;

private:
	void HandleResult(EGenerationActionResult GenerationActionResult, const FFaerieCraftingActionData& FaerieItemStacks);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Cancel();

protected:
	UPROPERTY(BlueprintAssignable)
	FFaerieAsyncCraftingActionCompleted GenerationActionCompleted;

	UPROPERTY()
	TObjectPtr<UObject> WorldContext;

	UPROPERTY()
	TInstancedStruct<FFaerieCraftingActionBase> Action;

	FFaerieCraftingActionHandle Handle;
};
