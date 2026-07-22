// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemCraftingRunner.h"
#include "FaerieItemGenerationLog.h"
#include "Squirrel.h"
#include "TimerManager.h"

#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemCraftingRunner)

using namespace Faerie;

void UFaerieItemCraftingRunner::BeginDestroy()
{
	CancelAllActions();
	Super::BeginDestroy();
}

void UFaerieItemCraftingRunner::SetSquirrel(USquirrel* InSquirrel)
{
	Squirrel = InSquirrel;
}

FFaerieCraftingActionHandle UFaerieItemCraftingRunner::SubmitCraftingRequest(
	TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback)
{
	if (!Request.IsValid())
	{
		return FFaerieCraftingActionHandle();
	}

	const auto CallbackWrapper = Generation::FActionResult::CreateWeakLambda(this,
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

TStructView<FFaerieCraftingActionBase> UFaerieItemCraftingRunner::GetRunningAction(const FFaerieCraftingActionHandle Handle)
{
	if (FFaeriePrivate_CapturedCraftingAction* ActionPtr = ActiveActions.FindByHash(Handle.Key, Handle))
	{
		return ActionPtr->Action;
	}
	return TStructView<FFaerieCraftingActionBase>();
}

void UFaerieItemCraftingRunner::CancelCraftingAction(const FFaerieCraftingActionHandle Handle)
{
	FinishAction(Handle, EGenerationActionResult::Cancelled);
}

void UFaerieItemCraftingRunner::CancelAllActions()
{
	if (!ActiveActions.IsEmpty())
	{
		for (auto&& ActiveAction : ActiveActions)
		{
			FinishActionImpl(ActiveAction.Action.GetMutable(), EGenerationActionResult::Cancelled);
		}
		ActiveActions.Empty();
	}
}

FFaerieCraftingActionHandle UFaerieItemCraftingRunner::SubmitCraftingAction_Impl(TInstancedStruct<FFaerieCraftingActionBase>& Action, const Generation::FActionResult* Callback)
{
	check(Action.GetScriptStruct() != FFaerieCraftingActionBase::StaticStruct());
	const FFaerieCraftingActionHandle Handle(FMath::Rand());
	Action.GetMutable().Handle = Handle;
	const FSetElementId ElementID = ActiveActions.AddByHash(Handle.Key, {Action, Handle});

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

	const Generation::FActionExecution Execution(
		this,
		GetWorld(),
		Squirrel
	);
	MutableAction.Run(Execution);

	return Handle;
}

#if WITH_EDITORONLY_DATA
void UFaerieItemCraftingRunner::LogActionResult(const FDateTime TimeStarted, const EGenerationActionResult Result, const FStringView ActionName)
{
	const FDateTime TimeFinished = FDateTime::UtcNow();
	const FTimespan TimePassed = TimeFinished - TimeStarted;

	switch (Result)
	{
	case EGenerationActionResult::Failed:
		UE_LOG(LogItemGeneration, Error, TEXT("+==+ Generation Action \"%s\" failed at: %s. Time passed: %s"), ActionName.GetData(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Timeout:
		UE_LOG(LogItemGeneration, Warning, TEXT("+==+ Generation Action \"%s\" timed-out at: %s. Time passed: %s"), ActionName.GetData(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Cancelled:
		UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" cancelled at: %s. Time passed: %s"), ActionName.GetData(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Succeeded:
		UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" succeeded at: %s. Time passed: %s"), ActionName.GetData(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	default: ;
	}
}
#endif

void UFaerieItemCraftingRunner::FinishAction(const FFaerieCraftingActionHandle Handle, const EGenerationActionResult Result)
{
	if (FFaeriePrivate_CapturedCraftingAction* ActionPtr = ActiveActions.FindByHash(Handle.Key, Handle))
	{
		FinishAction(TStructView<FFaerieCraftingActionBase>(ActionPtr->Action), Result);
	}
}

void UFaerieItemCraftingRunner::FinishAction(const TStructView<FFaerieCraftingActionBase> Action, const EGenerationActionResult Result)
{
#if WITH_EDITOR
	LogActionResult(Action->TimeStarted, Result, Action.GetScriptStruct()->GetName());
#endif
	FinishActionImpl(Action.Get(), Result);
	ActiveActions.RemoveByHash(Action->Handle.Key, Action->Handle);
}

void UFaerieItemCraftingRunner::FinishActionImpl(FFaerieCraftingActionBase& Action, const EGenerationActionResult Result) const
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
}
