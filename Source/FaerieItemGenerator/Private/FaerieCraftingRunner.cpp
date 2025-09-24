// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCraftingRunner.h"
#include "ItemSlotHandle.h"
#include "FaerieItem.h"
#include "FaerieItemCraftingSubsystem.h"
#include "Algo/ForEach.h"
#include "Engine/AssetManager.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemTemplate.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftingRunner)

UWorld* UFaerieCraftingRunner::GetWorld() const
{
	return GetOuterUFaerieItemCraftingSubsystem()->GetWorld();
}

FTimerManager& UFaerieCraftingRunner::GetTimerManager() const
{
	auto&& World = GetWorld();
	return World->GetTimerManager();
}

void UFaerieCraftingRunner::OnTimeout()
{
	Finish(EGenerationActionResult::Timeout);
}

void UFaerieCraftingRunner::Finish(const EGenerationActionResult Result)
{
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

	RunningRequest.Reset();

	if (RunningStreamHandle.IsValid())
	{
		RunningStreamHandle->CancelHandle();
	}

	GetTimerManager().ClearTimer(TimerHandle);

	(void)OnCompletedCallback.ExecuteIfBound(this, Result);
}

void UFaerieCraftingRunner::Start(const TInstancedStruct<FFaerieCraftingRequestBase>& Request)
{
#if WITH_EDITORONLY_DATA
	TimeStarted = FDateTime::UtcNow();

	UE_LOG(LogItemGeneration, Log, TEXT("+==+ Generation Action \"%s\" started at: %s"), *GetName(), *TimeStarted.ToString());
#endif

	RunningRequest = Request;

	GetTimerManager().SetTimer(TimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::OnTimeout), GetDefaultTimeoutTime(), false);

	RunningRequest.Get().Run(this);
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

namespace Faerie
{
	void ConsumeSlotCosts(const TMap<FFaerieItemSlotHandle, FFaerieItemProxy>& Slots, const IFaerieItemSlotInterface* Interface)
	{
		auto CanEat = [Slots](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
			{
				auto&& ItemProxy = *Slots.Find(Slot.Key);

				if (!ensure(ItemProxy.IsValid()))
				{
					UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to find a filled slot [%s]!"), *Slot.Key.ToString())
					return false;
				}

				if (!ItemProxy.IsInstanceMutable())
				{
					UE_LOG(LogItemGeneration, Error, TEXT("ConsumeSlotCosts is unable to mutate the item in slot [%s]!"), *Slot.Key.ToString())
					return false;
				}

				return true;
			};

		auto EatUse = [Slots](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
			{
				auto&& ItemProxy = *Slots.Find(Slot.Key);

				UFaerieItem* Item = ItemProxy->GetItemObject()->MutateCast();
				if (!ensure(IsValid(Item)))
				{
					return;
				}

				bool RemovedUse = false;

				// If the item can be used as a resource multiple times.
				if (Item->IsInstanceMutable())
				{
					if (auto&& Uses = Item->GetMutableToken<UFaerieItemUsesToken>())
					{
						RemovedUse = Uses->RemoveUses(1);
					}
				}

				// Otherwise, consume the item itself
				if (!RemovedUse)
				{
					(void)ItemProxy->GetItemOwner()->Release({Item, 1});
				}
			};

		const FFaerieCraftingSlotsView SlotsView = Crafting::GetCraftingSlots(Interface);
		const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get();

		Algo::ForEachIf(SlotsPtr.RequiredSlots, CanEat, EatUse);
		Algo::ForEachIf(SlotsPtr.OptionalSlots, CanEat, EatUse);
	}
}