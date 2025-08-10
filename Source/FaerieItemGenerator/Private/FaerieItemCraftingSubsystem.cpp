// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCraftingSubsystem)

void UFaerieItemCraftingSubsystem::Deinitialize()
{
	for (auto&& ActiveAction : ActiveActions)
	{
		ActiveAction->Cancel();
	}
	ActiveActions.Empty();

	Super::Deinitialize();
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
	const FFaerieCraftingActionData& ActionResults = Runner->RequestStorage.Get();
	Callback.Execute(Result, ActionResults.ProcessStacks);
	ActiveActions.Remove(Runner);
}