// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCraftingRunner.h"
#include "FaerieItemCraftingSubsystem.h"
#include "Engine/AssetManager.h"
#include "FaerieItemGenerationLog.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftingRunner)

void UFaerieCraftingRunner::Finish(const EGenerationActionResult Result)
{
	// If the output is invalid, we finished without crafting anything... so report a failure instead of intended result.
	if (!RequestStorage.IsValid())
	{
		(void)OnCompletedCallback.ExecuteIfBound(this, EGenerationActionResult::Failed);
		return;
	}

#if WITH_EDITORONLY_DATA
	const FDateTime TimeFinished = FDateTime::UtcNow();
	const FTimespan TimePassed = TimeFinished - TimeStarted;

	switch (Result)
	{
	case EGenerationActionResult::Failed:
		UE_LOG(LogItemGeneration, Error, TEXT("+==+ Generation Action \"%s\" failed at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Timeout:
		UE_LOG(LogItemGeneration, Warning, TEXT("+==+ Generation Action \"%s\" timed-out at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Cancelled:
		UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" cancelled at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Succeeded:
		UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" succeeded at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	default: ;
	}
#endif

	if (RunningStreamHandle.IsValid())
	{
		RunningStreamHandle->CancelHandle();
	}

	(void)OnCompletedCallback.ExecuteIfBound(this, Result);
}

void UFaerieCraftingRunner::Cancel()
{
	Finish(EGenerationActionResult::Cancelled);
}

void UFaerieCraftingRunner::Complete()
{
	Finish(EGenerationActionResult::Succeeded);
}

void UFaerieCraftingRunner::Fail()
{
	Finish(EGenerationActionResult::Failed);
}