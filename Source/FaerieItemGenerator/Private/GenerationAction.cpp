// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GenerationAction.h"
#include "ItemSlotHandle.h"
#include "FaerieItem.h"
#include "FaerieItemDataProxy.h"
#include "FaerieItemCraftingSubsystem.h"
#include "Algo/ForEach.h"
#include "Engine/AssetManager.h"
#include "DelegateCommon.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemTemplate.h"
#include "Tokens/FaerieItemUsesToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GenerationAction)

DEFINE_LOG_CATEGORY(LogGenerationAction)

UCraftingActionBase::UCraftingActionBase()
{
}

UWorld* UCraftingActionBase::GetWorld() const
{
	return GetOuterUFaerieItemCraftingSubsystem()->GetWorld();
}

bool UCraftingActionBase::IsRunning() const
{
	return TimerHandle.IsValid();
}

FTimerManager& UCraftingActionBase::GetTimerManager() const
{
	auto&& World = GetWorld();
	return World->GetTimerManager();
}

void UCraftingActionBase::OnTimeout()
{
#if WITH_EDITORONLY_DATA
	if (!IsRunning_EditorOnlyTracker)
	{
		UE_LOG(LogGenerationAction, Error, TEXT("UGenerationActionBase::OnTimeout() called without IsRunning true!"))
	}
#endif

	Finish(EGenerationActionResult::Timeout);
}

void UCraftingActionBase::Finish(const EGenerationActionResult Result)
{
#if WITH_EDITORONLY_DATA
	if (!IsRunning_EditorOnlyTracker)
	{
		UE_LOG(LogGenerationAction, Error, TEXT("Finish(...) was called on Generation Action without IsRunning true!"));
		return;
	}

	IsRunning_EditorOnlyTracker = false;

	const FDateTime TimeFinished = FDateTime::UtcNow();
	const FTimespan TimePassed = TimeFinished - TimeStarted;

	switch (Result)
	{
	case EGenerationActionResult::Failed:
		UE_LOG(LogGenerationAction, Error, TEXT("+==+ Generation Action \"%s\" failed at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Timeout:
		UE_LOG(LogGenerationAction, Warning, TEXT("+==+ Generation Action \"%s\" timed-out at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	case EGenerationActionResult::Succeeded:
		UE_LOG(LogGenerationAction, Log, TEXT("+==+ Generation Action \"%s\" succeeded at: %s. Time passed: %s"), *GetName(), *TimeFinished.ToString(), *TimePassed.ToString());
		break;
	default: ;
	}
#endif

	GetTimerManager().ClearTimer(TimerHandle);

	(void)OnCompletedCallback.ExecuteIfBound(Result);

	OnCompleted.Broadcast(Result, ProcessStacks);
}

void UCraftingActionBase::Configure(FActionArgs& Args)
{
	Squirrel = Args.Squirrel;
	OnCompleted.Add(DYNAMIC_TO_SCRIPT(Args.Callback));
}

void UCraftingActionBase::Start()
{
#if WITH_EDITORONLY_DATA
	if (IsRunning_EditorOnlyTracker)
	{
		UE_LOG(LogGenerationAction, Error, TEXT("Start() called on Generation Action that is already running!"))
		return Fail();
	}

	IsRunning_EditorOnlyTracker = true;

	TimeStarted = FDateTime::UtcNow();

	UE_LOG(LogGenerationAction, Log, TEXT("+==+ Generation Action \"%s\" started at: %s"), *GetName(), *TimeStarted.ToString());
#endif

	GetTimerManager().SetTimer(TimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::OnTimeout), GetDefaultTimeoutTime(), false);

	TArray<FSoftObjectPath> ObjectsToLoad = GetAssetsToLoad();

	if (ObjectsToLoad.IsEmpty())
	{
		return Run();
	}

	UE_LOG(LogGenerationAction, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check to HasBegunPlay forces this action to be ran in the editor synchronously
	if (GetWorld()->HasBegunPlay())
	{
		// Suspend generation to async load drop assets, then continue
		const FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &ThisClass::Run);
		UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad, Delegate);
	}
	else
	{
		// Immediately load all objects and continue.
		for (const FSoftObjectPath& Object : ObjectsToLoad)
		{
			Object.TryLoad();
		}

		Run();
	}
}

void UCraftingActionBase::Complete()
{
	Finish(EGenerationActionResult::Succeeded);
}

void UCraftingActionBase::Fail()
{
	Finish(EGenerationActionResult::Failed);
}

FFaerieItemProxy UCraftingActionWithSlots::GetProxyForSlot(const FFaerieItemSlotHandle& Filter, const bool ExpectPresence) const
{
	if (auto&& Key = FilledSlots.Find(Filter))
	{
		return *Key;
	}

	if (ExpectPresence)
	{
		UE_LOG(LogGenerationAction, Error,
			TEXT("<GetKeyForFilter> FilledSlots does not contain an entry for the requested slot ID: \'%s\'"), *Filter.ToString())
	}

	return nullptr;
}

void UCraftingActionWithSlots::ConsumeSlotCosts(const IFaerieItemSlotInterface* Interface)
{
	auto CanEat = [this](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
		{
			auto&& ItemProxy = *FilledSlots.Find(Slot.Key);

			if (!ensure(ItemProxy.IsValid()))
			{
				UE_LOG(LogGenerationAction, Error, TEXT("ConsumeSlotCosts is unable to find a filled slot [%s]!"), *Slot.Key.ToString())
				return false;
			}

			if (!ItemProxy.IsInstanceMutable())
			{
				UE_LOG(LogGenerationAction, Error, TEXT("ConsumeSlotCosts is unable to mutate the item in slot [%s]!"), *Slot.Key.ToString())
				return false;
			}

			return true;
		};

	auto EatUse = [this](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
		{
			auto&& ItemProxy = *FilledSlots.Find(Slot.Key);

			UFaerieItem* Item = ItemProxy->GetItemObject()->MutateCast();
			if (!ensure(IsValid(Item)))
			{
				return;
			}

			bool RemovedUse = false;

			// If the item can be used as a resource multiple times.
			if (Item->IsInstanceMutable())
			{
				if (auto&& Uses = Item->GetEditableToken<UFaerieItemUsesToken>())
				{
					RemovedUse = Uses->RemoveUses(1);
				}
			}

			// Otherwise, consume the item itself
			if (!RemovedUse)
			{
				(void)ItemProxy->GetOwner()->Release({Item, 1});
			}
		};

	const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(Interface);
	const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get();

	Algo::ForEachIf(SlotsPtr.RequiredSlots, CanEat, EatUse);
	Algo::ForEachIf(SlotsPtr.OptionalSlots, CanEat, EatUse);
}

void UCraftingActionWithSlots::Configure(FActionArgs& Args)
{
	RunConsumeStep = Args.RunConsumeStep;
	FilledSlots = Args.FilledSlots;
	Super::Configure(Args);
}

void UCraftingActionWithSlots::Run()
{
	for (auto&& Element : FilledSlots)
	{
		if (!Element.Value.IsValid() ||
			!IsValid(Element.Value->GetItemObject()) ||
			!Element.Value.IsInstanceMutable())
		{
			UE_LOG(LogGenerationAction, Error, TEXT("A filled slot [%s] is invalid!)"), *Element.Key.ToString())
			Fail();
		}
	}
}