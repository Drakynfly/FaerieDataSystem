// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "ItemCraftingRunner.generated.h"

namespace Faerie::Generation
{
	template <typename T>
	concept CCraftingAction = TIsDerivedFrom<typename TRemoveReference<T>::Type, FFaerieCraftingActionBase>::Value;
}

DECLARE_DYNAMIC_DELEGATE_TwoParams(FGenerationActionOnCompleteBinding, EGenerationActionResult, Result,
								   const FFaerieCraftingActionData&, Data);

USTRUCT()
struct FFaeriePrivate_CapturedCraftingAction
{
	GENERATED_BODY()

	UPROPERTY()
	TInstancedStruct<FFaerieCraftingActionBase> Action;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaeriePrivate_CapturedCraftingAction& Other) const
	{
		return Action.Get().GetHandle() == Other.Action.Get().GetHandle();
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieCraftingActionHandle& Other) const
	{
		return Action.Get().GetHandle() == Other;
	}

	friend uint32 GetTypeHash(const FFaeriePrivate_CapturedCraftingAction& Value)
	{
		return GetTypeHash(Value.Action.Get().GetHandle());
	}
};

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemCraftingRunner : public UObject
{
	GENERATED_BODY()

	friend FFaerieCraftingActionBase;

public:
	template <Faerie::Generation::CCraftingAction T>
	FFaerieCraftingActionHandle SubmitCraftingAction(T&& Action)
	{
		auto ActionStruct = TInstancedStruct<FFaerieCraftingActionBase>::Make<T>(Action);
		return SubmitCraftingAction_Impl(ActionStruct, nullptr);
	}

	template <Faerie::Generation::CCraftingAction T>
	FFaerieCraftingActionHandle SubmitCraftingAction(T&& Action, const Faerie::Generation::FActionResult& Callback)
	{
		auto ActionStruct = TInstancedStruct<FFaerieCraftingActionBase>::Make<T>(Action);
		return SubmitCraftingAction_Impl(ActionStruct, &Callback);
	}

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemGeneration")
	FFaerieCraftingActionHandle SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback);

	FFaerieCraftingActionHandle SubmitCraftingAction(TInstancedStruct<FFaerieCraftingActionBase>& Action, const Faerie::Generation::FActionResult& Callback);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void CancelCraftingAction(FFaerieCraftingActionHandle Handle);

	TInstancedStruct<FFaerieCraftingActionBase>* GetRunningAction(FFaerieCraftingActionHandle Handle);

	void CancelAllActions();

private:
	void CompleteCraftingAction(FFaerieCraftingActionHandle Handle);

	void FailCraftingAction(FFaerieCraftingActionHandle Handle);

	FFaerieCraftingActionHandle SubmitCraftingAction_Impl(TInstancedStruct<FFaerieCraftingActionBase>& Action, const Faerie::Generation::FActionResult* Callback);

	void FinishAction(FFaerieCraftingActionHandle Handle, EGenerationActionResult Result);
	void FinishActionImpl(FFaerieCraftingActionBase& Action, EGenerationActionResult Result);

private:
	// The Actions currently running.
	UPROPERTY(Transient)
	TSet<FFaeriePrivate_CapturedCraftingAction> ActiveActions;

	float ActionTimeoutDuration = 30.f;
};