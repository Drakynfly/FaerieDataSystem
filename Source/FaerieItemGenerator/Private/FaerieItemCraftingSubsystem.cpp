﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"

#include "FaerieItemTemplate.h"
#include "ItemCraftingConfig.h"
#include "ItemUpgradeConfig.h"
#include "ItemSlotHandle.h"

#include "GenerationAction.h"
#include "GenerationAction_CraftItems.h"
#include "GenerationAction_GenerateItems.h"
#include "GenerationAction_UpgradeItems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCraftingSubsystem)

DEFINE_LOG_CATEGORY(LogItemGeneratorSubsystem)

void UFaerieItemCraftingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Initialized = true;
}

void UFaerieItemCraftingSubsystem::BeginRunningAction(UCraftingActionBase* Action)
{
	check(Action);
	ActiveAction = Action;
	ActiveAction->OnCompletedCallback.BindUObject(this, &ThisClass::OnActionCompleted);
	ActiveAction->Start();
}

void UFaerieItemCraftingSubsystem::EnqueueAction_Internal(UCraftingActionBase* NewAction)
{
	if (!IsValid(ActiveAction))
	{
		BeginRunningAction(NewAction);
		return;
	}

	PendingActions.Enqueue(TStrongObjectPtr<UCraftingActionBase>(NewAction));
}

void UFaerieItemCraftingSubsystem::OnActionCompleted(EGenerationActionResult /*Result*/)
{
	if (!PendingActions.IsEmpty())
	{
		TStrongObjectPtr<UCraftingActionBase> Next;
		PendingActions.Dequeue(Next);
		if (Next.IsValid())
		{
			BeginRunningAction(Next.Get());
		}
	}
	else
	{
		ActiveAction = nullptr;
	}
}

void UFaerieItemCraftingSubsystem::SubmitGenerationRequest(const FGenerationRequest& Request)
{
	if (!IsValid(Request.Executor))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Executor is invalid!"));
		return;
	}

	if (Request.Drivers.IsEmpty())
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Drivers are empty!"));
		return;
	}

	for (auto&& Driver : Request.Drivers)
	{
		if (!Driver)
		{
			UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Driver is misconfigured!"));
			return;
		}
	}

	UGenerationAction_GenerateItems::FActionArgs Args;
	Args.Executor = Request.Executor;
	Args.Callback = Request.OnComplete;
	Args.Drivers = Request.Drivers;

	EnqueueActionTyped<UGenerationAction_GenerateItems>(Args);
}

void UFaerieItemCraftingSubsystem::SubmitUpgradeRequest(const FUpgradeRequest& Request)
{
	if (!IsValid(Request.Executor))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Executor is invalid!"));
		return;
	}

	if (!IsValid(Request.ItemProxy.GetObject()))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": ItemProxy is invalid!"));
		return;
	}

	if (!IsValid(Request.Config))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Config is invalid!"));
		return;
	}

	UGenerationAction_UpgradeItems::FActionArgs Args;
	Args.Executor = Request.Executor;
	Args.Callback = Request.OnComplete;
	Args.UpgradeConfig = Request.Config;
	Args.ItemBeingUpgraded = Request.ItemProxy;

	const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(Request.Config);
	const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get<const FFaerieItemCraftingSlots>();

	for (auto&& RequiredSlot : SlotsPtr.RequiredSlots)
	{
		if (auto&& SlotPtr = Request.Slots.FindByPredicate(
			[RequiredSlot](const FRequestSlot& Slot)
			{
				return Slot.SlotID == RequiredSlot.Key;
			}))
		{
			if (!IsValid(SlotPtr->ItemProxy.GetObject()))
			{
				UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Proxy is invalid for slot: %s!"), *RequiredSlot.Key.ToString());
				return;
			}

			if (RequiredSlot.Value->TryMatch(SlotPtr->ItemProxy))
			{
				Args.FilledSlots.Add(RequiredSlot.Key, SlotPtr->ItemProxy);
			}
			else
			{
				UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Required Slot '%s'failed with key: %s"),
					*SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
				return;
			}
		}
		else
		{
			UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Request does not contain required slot: %s!"), *RequiredSlot.Key.ToString());
			return;
		}
	}

	for (auto&& OptionalSlot : SlotsPtr.OptionalSlots)
	{
		if (auto&& SlotPtr = Request.Slots.FindByPredicate(
			[OptionalSlot](const FRequestSlot& Slot)
			{
				return Slot.SlotID == OptionalSlot.Key;
			}))
		{
			if (!IsValid(SlotPtr->ItemProxy.GetObject()))
			{
				UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Entry is invalid for slot: %s!"), *OptionalSlot.Key.ToString());
				return;
			}

			if (OptionalSlot.Value->TryMatch(SlotPtr->ItemProxy))
			{
				Args.FilledSlots.Add(OptionalSlot.Key, SlotPtr->ItemProxy);
			}
			else
			{
				UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Optional Slot '%s' failed with key: %s"),
					*SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
				return;
			}
		}
	}

	// With all slots validated, execute the upgrade.
	EnqueueActionTyped<UGenerationAction_UpgradeItems>(Args);
}

void UFaerieItemCraftingSubsystem::SubmitCraftingRequest_Impl(const FCraftingRequest& Request, const bool Preview)
{
	if (!IsValid(Request.Executor))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Executor is invalid!"));
		return;
	}

	if (!IsValid(Request.Config))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Config is invalid!"));
		return;
	}

	if (!IsValid(Request.Executor))
	{
		UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": ItemOuter is invalid!"));
		return;
	}

	UGenerationAction_CraftItems::FActionArgs Args;
	Args.Executor = Request.Executor;
	Args.Callback = Request.OnComplete;
	Args.CraftConfig = Request.Config;
	Args.RunConsumeStep = !Preview;

	if (const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(Request.Config);
		SlotsView.IsValid())
	{
		const FFaerieItemCraftingSlots& Slots = SlotsView.Get<const FFaerieItemCraftingSlots>();

		for (auto&& RequiredSlot : Slots.RequiredSlots)
		{
			if (auto&& SlotPtr = Request.Slots.FindByPredicate(
				[RequiredSlot](const FRequestSlot& Slot)
				{
					return Slot.SlotID == RequiredSlot.Key;
				}))
			{
				if (!IsValid(SlotPtr->ItemProxy.GetObject()))
				{
					UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Entry is invalid for slot: %s!"), *RequiredSlot.Key.ToString());
					return;
				}

				if (RequiredSlot.Value->TryMatch(SlotPtr->ItemProxy))
				{
					Args.FilledSlots.Add(RequiredSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Required Slot '%s' failed with key: %s"),
						*SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return;
				}
			}
			else
			{
				UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Request does contain required slot: %s!"), *RequiredSlot.Key.ToString());
				return;
			}
		}

		for (auto&& OptionalSlot : Slots.OptionalSlots)
		{
			if (auto&& SlotPtr = Request.Slots.FindByPredicate(
				[OptionalSlot](const FRequestSlot& Slot)
				{
					return Slot.SlotID == OptionalSlot.Key;
				}))
			{
				if (!IsValid(SlotPtr->ItemProxy.GetObject()))
				{
					UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT(": Entry is invalid for slot: %s!"), *OptionalSlot.Key.ToString());
					return;
				}

				if (OptionalSlot.Value->TryMatch(SlotPtr->ItemProxy))
				{
					Args.FilledSlots.Add(OptionalSlot.Key, SlotPtr->ItemProxy);
				}
				else
				{
					UE_LOG(LogItemGeneratorSubsystem, Warning, __FUNCTION__ TEXT("Optional Slot '%s' failed with key: %s"),
						*SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
					return;
				}
			}
		}
	}

	// With all slots validated, execute the upgrade.
	EnqueueActionTyped<UGenerationAction_CraftItems>(Args);
}

void UFaerieItemCraftingSubsystem::SubmitCraftingRequest(const FCraftingRequest& Request)
{
	SubmitCraftingRequest_Impl(Request, false);
}

void UFaerieItemCraftingSubsystem::PreviewCraftingRequest(const FCraftingRequest& Request)
{
	SubmitCraftingRequest_Impl(Request, true);
}