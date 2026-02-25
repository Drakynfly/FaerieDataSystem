// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.


#include "ItemCraftingRunner.h"
#include "FaerieItemGenerationLog.h"
#include "Engine/StreamableManager.h"

using namespace Faerie;

FFaerieCraftingActionHandle UFaerieItemCraftingRunner::SubmitCraftingRequest(
	TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback)
{
	if (!Request.IsValid())
	{
		return FFaerieCraftingActionHandle();
	}

	const auto CallbackWrapper = Generation::FActionResult::CreateLambda(
		[Callback](const EGenerationActionResult Success, const FFaerieCraftingActionData& Stacks)
		{
			Callback.Execute(Success, Stacks);
		});

	return SubmitCraftingAction_Impl(Request, &CallbackWrapper);
}

FFaerieCraftingActionHandle UFaerieItemCraftingRunner::SubmitCraftingAction(
	TInstancedStruct<FFaerieCraftingActionBase>& Action, const Generation::FActionResult& Callback)
{
	if (!Action.IsValid())
	{
		return FFaerieCraftingActionHandle();
	}

	return SubmitCraftingAction_Impl(Action, &Callback);
}

void UFaerieItemCraftingRunner::CancelCraftingAction(const FFaerieCraftingActionHandle Handle)
{
	FinishAction(Handle, EGenerationActionResult::Cancelled);
}

TInstancedStruct<FFaerieCraftingActionBase>* UFaerieItemCraftingRunner::GetRunningAction(const FFaerieCraftingActionHandle Handle)
{
	if (FFaeriePrivate_CapturedCraftingAction* ActionPtr = ActiveActions.FindByHash(Handle.Key, Handle))
	{
		return &ActionPtr->Action;
	}
	return nullptr;
}

void UFaerieItemCraftingRunner::CancelAllActions()
{
	if (!ActiveActions.IsEmpty())
	{
		auto ActionsCopy = ActiveActions;
		for (auto&& ActiveAction : ActionsCopy)
		{
			FinishActionImpl(ActiveAction.Action.GetMutable(), EGenerationActionResult::Cancelled);
		}
		ensure(ActiveActions.IsEmpty());
	}
}
void UFaerieItemCraftingRunner::CompleteCraftingAction(const FFaerieCraftingActionHandle Handle)
{
	FinishAction(Handle, EGenerationActionResult::Succeeded);
}

void UFaerieItemCraftingRunner::FailCraftingAction(const FFaerieCraftingActionHandle Handle)
{
	FinishAction(Handle, EGenerationActionResult::Failed);
}

FFaerieCraftingActionHandle UFaerieItemCraftingRunner::SubmitCraftingAction_Impl(TInstancedStruct<FFaerieCraftingActionBase>& Action, const Generation::FActionResult* Callback)
{
	check(Action.GetScriptStruct() != FFaerieCraftingActionBase::StaticStruct());
	FFaerieCraftingActionBase& Prep = Action.GetMutable();
	const FFaerieCraftingActionHandle Handle(FMath::Rand());
	Prep.Handle = Handle;
	const FSetElementId ElementID = ActiveActions.AddByHash(Handle.Key, {Action});

	check(ActiveActions[ElementID].Action.GetScriptStruct() != FFaerieCraftingActionBase::StaticStruct());

	FFaerieCraftingActionBase& MutableAction = ActiveActions[ElementID].Action.GetMutable();

	if (Callback)
	{
		MutableAction.OnCompletedCallback = *Callback;
	}

	GetWorld()->GetTimerManager().SetTimer(MutableAction.TimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::FinishAction, Handle, EGenerationActionResult::Timeout), ActionTimeoutDuration, false);

#if WITH_EDITORONLY_DATA
	MutableAction.TimeStarted = FDateTime::UtcNow();

	UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" started at: %s"), *Action.GetScriptStruct()->GetName(), *MutableAction.TimeStarted.ToString());
#endif

	MutableAction.Run(this);

	return Handle;
}

void UFaerieItemCraftingRunner::FinishAction(const FFaerieCraftingActionHandle Handle, const EGenerationActionResult Result)
{
	if (FFaeriePrivate_CapturedCraftingAction* ActionPtr = ActiveActions.FindByHash(Handle.Key, Handle))
	{
#if WITH_EDITORONLY_DATA
		const FDateTime TimeFinished = FDateTime::UtcNow();
		const FTimespan TimePassed = TimeFinished - ActionPtr->Action.Get().TimeStarted;

		switch (Result)
		{
		case EGenerationActionResult::Failed:
			UE_LOG(LogItemGeneration, Error, TEXT("+==+ Generation Action \"%s\" failed at: %s. Time passed: %s"), *ActionPtr->Action.GetScriptStruct()->GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
			break;
		case EGenerationActionResult::Timeout:
			UE_LOG(LogItemGeneration, Warning, TEXT("+==+ Generation Action \"%s\" timed-out at: %s. Time passed: %s"), *ActionPtr->Action.GetScriptStruct()->GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
			break;
		case EGenerationActionResult::Cancelled:
			UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" cancelled at: %s. Time passed: %s"), *ActionPtr->Action.GetScriptStruct()->GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
			break;
		case EGenerationActionResult::Succeeded:
			UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" succeeded at: %s. Time passed: %s"), *ActionPtr->Action.GetScriptStruct()->GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
			break;
		default: ;
		}
#endif

		FinishActionImpl(ActionPtr->Action.GetMutable(), Result);
	}
}

void UFaerieItemCraftingRunner::FinishActionImpl(FFaerieCraftingActionBase& Action, const EGenerationActionResult Result)
{
	// Cancel any load this action had triggered
	if (Action.RunningStreamHandle.IsValid())
	{
		Action.RunningStreamHandle->CancelHandle();
	}

	// Remove the timeout timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Action.TimerHandle);
	}

	if (Action.ActionData.Stacks.IsEmpty())
	{
		// If the output is invalid, we finished without crafting anything... so report a failure instead of intended result.
		Action.OnCompletedCallback.ExecuteIfBound(EGenerationActionResult::Failed, {});
	}
	else if (Result == EGenerationActionResult::Succeeded)
	{
		Action.OnCompletedCallback.ExecuteIfBound(Result, Action.ActionData);
	}
	else
	{
		Action.OnCompletedCallback.ExecuteIfBound(Result, {});
	}

	ActiveActions.RemoveByHash(Action.Handle.Key, Action.Handle);
}
