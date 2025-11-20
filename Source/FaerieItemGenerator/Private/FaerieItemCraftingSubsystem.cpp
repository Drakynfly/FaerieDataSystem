// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCraftingSubsystem)

void UFaerieItemCraftingSubsystem::Deinitialize()
{
	if (!ActiveActions.IsEmpty())
	{
		auto ActionsCopy = ActiveActions;
		for (auto&& ActiveAction : ActionsCopy)
		{
			ActiveAction->Cancel();
		}
		ensure(ActiveActions.IsEmpty());
	}

	Super::Deinitialize();
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(const FFaerieCraftingRequestBase& Request, const FRequestResult& Callback)
{
	UFaerieCraftingRunner* ActiveAction = NewObject<UFaerieCraftingRunner>(this);
	ActiveActions.Add(ActiveAction);
	ActiveAction->GetOnCompletedCallback().BindUObject(this, &ThisClass::OnActionCompleted, Callback);
	ActiveAction->Start(TInstancedStruct<FFaerieCraftingRequestBase>::Make(Request));
	return ActiveAction;
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(
	const TInstancedStruct<FFaerieCraftingRequestBase> Request, const FGenerationActionOnCompleteBinding& Callback)
{
	if (!Request.IsValid())
	{
		return nullptr;
	}

	UFaerieCraftingRunner* ActiveAction = NewObject<UFaerieCraftingRunner>(this);
	ActiveActions.Add(ActiveAction);
	ActiveAction->GetOnCompletedCallback().BindUObject(this, &ThisClass::OnActionCompleted,
		FRequestResult::CreateLambda([Callback](const EGenerationActionResult Success, const TArray<FFaerieItemStack>& Stacks)
		{
			Callback.Execute(Success, Stacks);
		}));
	ActiveAction->Start(Request);
	return ActiveAction;
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(
	const TInstancedStruct<FFaerieCraftingRequestBase>& Request, FRequestResult Callback)
{
	if (!Request.IsValid())
	{
		return nullptr;
	}

	UFaerieCraftingRunner* ActiveAction = NewObject<UFaerieCraftingRunner>(this);
	ActiveActions.Add(ActiveAction);
	ActiveAction->GetOnCompletedCallback().BindUObject(this, &ThisClass::OnActionCompleted, Callback);
	ActiveAction->Start(Request);
	return ActiveAction;
}

void UFaerieItemCraftingSubsystem::OnActionCompleted(UFaerieCraftingRunner* Runner, const EGenerationActionResult Result,
	const FRequestResult Callback)
{
	if (Result == EGenerationActionResult::Succeeded)
	{
		const FFaerieCraftingActionData& ActionResults = Runner->RequestStorage.Get();
		Callback.Execute(Result, ActionResults.ProcessStacks);
	}
	else
	{
		Callback.Execute(Result, {});
	}

	ActiveActions.Remove(Runner);
}