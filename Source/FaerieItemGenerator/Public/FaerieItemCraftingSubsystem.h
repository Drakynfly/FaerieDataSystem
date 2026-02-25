// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "ItemCraftingAction.h"
#include "ItemCraftingRunner.h"
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
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFaerieItemCraftingRunner* GetRunner() const { return Runner; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemGeneration")
	FFaerieCraftingActionHandle SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void CancelCraftingAction(FFaerieCraftingActionHandle Handle);

private:
	UPROPERTY()
	TObjectPtr<UFaerieItemCraftingRunner> Runner;
};