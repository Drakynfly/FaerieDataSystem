// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeRequest.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemMutator.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"
#include "Squirrel.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Upgrades/FaerieItemUpgradeConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeRequest)

void FFaerieItemUpgradeRequest::Run(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(ItemProxy.GetObject()))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: ItemProxy is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(Config);
	const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get();

	FFaerieCraftingActionSlots SlotMemory;

	for (auto&& RequiredSlot : SlotsPtr.RequiredSlots)
	{
		if (auto&& SlotPtr = Slots.FindByPredicate(
			[RequiredSlot](const FFaerieCraftingRequestSlot& Slot)
			{
				return Slot.SlotID == RequiredSlot.Key;
			}))
		{
			if (!IsValid(SlotPtr->ItemProxy.GetObject()))
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Proxy is invalid for slot: %s!"),
					__FUNCTION__, *RequiredSlot.Key.ToString());
				return Runner->Fail();
			}

			if (RequiredSlot.Value->TryMatch(SlotPtr->ItemProxy))
			{
				SlotMemory.FilledSlots.Add(RequiredSlot.Key, SlotPtr->ItemProxy);
			}
			else
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Required Slot '%s' failed with key: %s"),
					__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
				return Runner->Fail();
			}
		}
		else
		{
			UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Request does not contain required slot: %s!"),
				__FUNCTION__, *RequiredSlot.Key.ToString());
			return Runner->Fail();
		}
	}

	for (auto&& OptionalSlot : SlotsPtr.OptionalSlots)
	{
		if (auto&& SlotPtr = Slots.FindByPredicate(
			[OptionalSlot](const FFaerieCraftingRequestSlot& Slot)
			{
				return Slot.SlotID == OptionalSlot.Key;
			}))
		{
			if (!IsValid(SlotPtr->ItemProxy.GetObject()))
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Entry is invalid for slot: %s!"),
					__FUNCTION__, *OptionalSlot.Key.ToString());
				return Runner->Fail();
			}

			if (OptionalSlot.Value->TryMatch(SlotPtr->ItemProxy))
			{
				SlotMemory.FilledSlots.Add(OptionalSlot.Key, SlotPtr->ItemProxy);
			}
			else
			{
				UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Optional Slot '%s' failed with key: %s"),
					__FUNCTION__, *SlotPtr->SlotID.ToString(), *SlotPtr->ItemProxy.GetObject()->GetName());
				return Runner->Fail();
			}
		}
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionSlots>(SlotMemory);

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->Mutator->GetRequiredAssets(RequiredAssets);

	for (auto&& RequiredAsset : RequiredAssets)
	{
		ObjectsToLoad.Add(RequiredAsset.ToSoftObjectPath());
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing to wait for, run now!
		return Execute(Runner);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsGameWorld forces this action to be ran in the editor synchronously
	if (!Runner->GetWorld()->IsGameWorld())
	{
		// Immediately load all objects and continue.
		for (const FSoftObjectPath& Object : ObjectsToLoad)
		{
			Object.TryLoad();
		}

		return Execute(Runner);
	}

	// Suspend generation to async load drop assets, then continue
	Runner->RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeRequest::Execute, Runner));
}

void FFaerieItemUpgradeRequest::Execute(UFaerieCraftingRunner* Runner) const
{
	FFaerieCraftingActionSlots& RequestData = Runner->RequestStorage.GetMutable<FFaerieCraftingActionSlots>();

	// Execute parent Run, as it validates some stuff, and then early out if it fails.
	for (auto&& Element : RequestData.FilledSlots)
	{
		if (!Element.Value.IsValid() ||
			!IsValid(Element.Value->GetItemObject()) ||
			!Element.Value.IsInstanceMutable())
		{
			UE_LOG(LogItemGeneration, Error, TEXT("A filled slot [%s] is invalid!)"), *Element.Key.ToString())
			Runner->Fail();
		}
	}

	// @todo batching
	int32 Copies = 1;

	const FFaerieItemStackView ReleaseRequest{ItemProxy->GetItemObject(), Copies};
	const FFaerieItemStack Stack = ItemProxy->GetItemOwner()->Release(ReleaseRequest);

	if (Stack.Copies == 0)
	{
		return Runner->Fail();
	}

	// Apply the mutator
	if (!Config->Mutator->TryApply(Stack, &Squirrel.Get()->GetState()))
	{
		return Runner->Fail();
	}

	RequestData.ProcessStacks.Add(Stack);

	if (RunConsumeStep)
	{
		if (Config->Mutator->Implements<UFaerieItemSlotInterface>())
		{
			Faerie::ConsumeSlotCosts(RequestData.FilledSlots, Cast<IFaerieItemSlotInterface>(Config->Mutator));
		}
	}

	Runner->Complete();
}