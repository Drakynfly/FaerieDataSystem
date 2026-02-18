// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "FaerieCraftingRunner.h"
#include "StructUtils/InstancedStruct.h"
#include "FaerieItemCraftingSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGenerationActionOnCompleteBinding, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);

namespace Faerie::Generation
{
	template <typename T>
    concept CCraftingAction = TIsDerivedFrom<typename TRemoveReference<T>::Type, FFaerieCraftingActionBase>::Value;
}

USTRUCT()
struct FFaerieCraftingActionHandle
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Key = 0;
};

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

	UFaerieCraftingRunner* SubmitCraftingRequest(const Faerie::Generation::CCraftingAction auto& Request)
	{
		auto ActionStruct = TInstancedStruct<FFaerieCraftingActionBase>::Make(Request);
		return SubmitCraftingRequest_Impl(ActionStruct, nullptr);
	}

	UFaerieCraftingRunner* SubmitCraftingRequest(const Faerie::Generation::CCraftingAction auto& Request, const FRequestResult& Callback)
	{
		auto ActionStruct = TInstancedStruct<FFaerieCraftingActionBase>::Make(Request);
		return SubmitCraftingRequest_Impl(ActionStruct, &Callback);
	}

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemGeneration")
	UFaerieCraftingRunner* SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback);

	UFaerieCraftingRunner* SubmitCraftingRequest(const TInstancedStruct<FFaerieCraftingActionBase>& Request, const FRequestResult& Callback);

private:
	UFaerieCraftingRunner* SubmitCraftingRequest_Impl(const TInstancedStruct<FFaerieCraftingActionBase>& Request, const FRequestResult* Callback);

	void OnActionTimeout(UFaerieCraftingRunner* Runner);
	void OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult Result);
	void OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult Result, FRequestResult Callback);

private:
	// The Actions currently running.
	UPROPERTY(Transient)
	TSet<TObjectPtr<UFaerieCraftingRunner>> ActiveActions;

	float ActionTimeoutDuration = 30.f;
};