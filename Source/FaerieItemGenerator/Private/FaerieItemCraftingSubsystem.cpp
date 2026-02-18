// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"
#include "FaerieItemGenerationLog.h"

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

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(
	const TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback)
{
	if (!Request.IsValid())
	{
		return nullptr;
	}

	const FRequestResult CallbackWrapper = FRequestResult::CreateLambda([Callback](const EGenerationActionResult Success, const TArray<FFaerieItemStack>& Stacks)
		{
			Callback.Execute(Success, Stacks);
		});

	return SubmitCraftingRequest_Impl(Request, &CallbackWrapper);
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(
	const TInstancedStruct<FFaerieCraftingActionBase>& Request, const FRequestResult& Callback)
{
	if (!Request.IsValid())
	{
		return nullptr;
	}

	return SubmitCraftingRequest_Impl(Request, &Callback);
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest_Impl(const TInstancedStruct<FFaerieCraftingActionBase>& Request, const FRequestResult* Callback)
{
	UFaerieCraftingRunner* ActiveAction = NewObject<UFaerieCraftingRunner>(this);
	ActiveActions.Add(ActiveAction);
	if (Callback)
	{
		ActiveAction->OnCompletedCallback.BindUObject(this, &ThisClass::OnActionCompleted, *Callback);
	}
	else
	{
		ActiveAction->OnCompletedCallback.BindUObject(this, &ThisClass::OnActionCompleted);
	}

	GetWorld()->GetTimerManager().SetTimer(ActiveAction->TimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::OnActionTimeout, ActiveAction), ActionTimeoutDuration, false);

#if WITH_EDITORONLY_DATA
	ActiveAction->TimeStarted = FDateTime::UtcNow();

	UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" started at: %s"), *GetName(), *ActiveAction->TimeStarted.ToString());
#endif

	Request.Get().Run(ActiveAction);

	return ActiveAction;
}

void UFaerieItemCraftingSubsystem::OnActionTimeout(UFaerieCraftingRunner* Runner)
{
	Runner->Finish(EGenerationActionResult::Timeout);
}

void UFaerieItemCraftingSubsystem::OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult Result)
{
	GetWorld()->GetTimerManager().ClearTimer(Runner->TimerHandle);
	ActiveActions.Remove(Runner);
}

void UFaerieItemCraftingSubsystem::OnActionCompleted(UFaerieCraftingRunner* Runner, const EGenerationActionResult Result,
													 const FRequestResult Callback)
{
	GetWorld()->GetTimerManager().ClearTimer(Runner->TimerHandle);

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
