// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "StructUtils/InstancedStruct.h"
#include "StructUtils/StructView.h"

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

	FFaerieCraftingActionHandle Handle;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaeriePrivate_CapturedCraftingAction& Other) const
	{
		return Handle == Other.Handle;
	}

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieCraftingActionHandle& Other) const
	{
		return Handle == Other;
	}

	friend uint32 GetTypeHash(const FFaeriePrivate_CapturedCraftingAction& Value)
	{
		return GetTypeHash(Value.Handle);
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
	virtual void BeginDestroy() override;

	void SetSquirrel(USquirrel* InSquirrel);

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

	TStructView<FFaerieCraftingActionBase> GetRunningAction(FFaerieCraftingActionHandle Handle);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void CancelCraftingAction(FFaerieCraftingActionHandle Handle);

	void CancelAllActions();

private:
	FFaerieCraftingActionHandle SubmitCraftingAction_Impl(TInstancedStruct<FFaerieCraftingActionBase>& Action, const Faerie::Generation::FActionResult* Callback);

#if WITH_EDITOR
	static void LogActionResult(const FDateTime TimeStarted, const EGenerationActionResult Result, FStringView ActionName, const FText& Message);
#endif

	void FinishAction(FFaerieCraftingActionHandle Handle, EGenerationActionResult Result);
	void FinishAction(TStructView<FFaerieCraftingActionBase> Action, EGenerationActionResult Result, const FText& Message);
	void FinishActionImpl(FFaerieCraftingActionBase& Action, EGenerationActionResult Result) const;

private:
	UPROPERTY()
	TObjectPtr<USquirrel> Squirrel;

	// The Actions currently running.
	UPROPERTY(Transient)
	TSet<FFaeriePrivate_CapturedCraftingAction> ActiveActions;

	float ActionTimeoutDuration = 30.f;
};