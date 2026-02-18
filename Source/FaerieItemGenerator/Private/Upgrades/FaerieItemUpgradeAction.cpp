// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeAction.h"
#include "Upgrades/FaerieItemUpgradeConfig.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemMutator.h"
#include "FaerieItemOwnerInterface.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemStackView.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeAction)

namespace Faerie::Generation
{
	bool ApplyConfigToProcessStacks(const TArrayView<FFaerieItemStack> Stacks, const TNotNull<UFaerieItemUpgradeConfig*> Config, FFaerieItemMutatorContext* Context)
	{
		for (auto&& OperationStack : Stacks)
		{
			if (OperationStack.Copies == 0)
			{
				return false;
			}

			// Apply the mutator, and fail if it doesn't apply, when RequireMutatorToRun is enabled.
			if (!Config->Mutator.Get().Apply(OperationStack, Context) && Config->RequireMutatorToRun)
			{
				return false;
			}
		}
		return true;
	}
}

void FFaerieItemUpgradeAction::Run(UFaerieCraftingRunner* Runner) const
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

	FFaerieCraftingActionData SlotMemory;

	if (const FFaerieCraftingSlotsView CraftingSlots = Config->GetCraftingSlots();
		CraftingSlots.IsValid() && !Faerie::Generation::ValidateFilledSlots(Slots, CraftingSlots.Get()))
	{
		return Runner->Fail();
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionData>(SlotMemory);

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->Mutator.Get().GetRequiredAssets(RequiredAssets);

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
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeAction::Execute, Runner));
}

void FFaerieItemUpgradeAction::Execute(UFaerieCraftingRunner* Runner) const
{
	FFaerieCraftingActionData& RequestData = Runner->RequestStorage.GetMutable<FFaerieCraftingActionData>();

	// @todo batching
	int32 Copies = 1;

	if (Config->ReleaseWhileOperating)
	{
		RequestData.ProcessStacks.Add(ItemProxy->Release(Copies));
	}
	else
	{
		RequestData.ProcessStacks.Add(FFaerieItemStack(ItemProxy.GetItemObject(), Copies));
	}

	FFaerieItemMutatorContext Context;
	Context.Squirrel = Squirrel.Get();

	if (!Faerie::Generation::ApplyConfigToProcessStacks(RequestData.ProcessStacks, Config, &Context))
	{
		return Runner->Fail();
	}

	if (RunConsumeStep)
	{
		if (Config->Implements<UFaerieItemSlotInterface>())
		{
			if (const FFaerieCraftingSlotsView SlotsView = Faerie::Generation::GetCraftingSlots(Config);
				SlotsView.IsValid())
			{
				Faerie::Generation::ConsumeSlotCosts(Slots, SlotsView.Get());
			}
		}
	}

	Runner->Complete();
}

void FFaerieItemUpgradeActionBulkNoPayment::Run(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	FFaerieCraftingActionData ActionData;
	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionData>(ActionData);

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->Mutator.Get().GetRequiredAssets(RequiredAssets);

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
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulkNoPayment::Execute, Runner));
}

void FFaerieItemUpgradeActionBulkNoPayment::Execute(UFaerieCraftingRunner* Runner) const
{
	FFaerieCraftingActionData& RequestData = Runner->RequestStorage.GetMutable<FFaerieCraftingActionData>();

	// Prepare ProcessStacks
	RequestData.ProcessStacks = UpgradeTargets;

	FFaerieItemMutatorContext Context;
	Context.Squirrel = Squirrel.Get();

	if (!Faerie::Generation::ApplyConfigToProcessStacks(RequestData.ProcessStacks, Config, &Context))
	{
		return Runner->Fail();
	}

	Runner->Complete();
}

void FFaerieItemUpgradeActionBulk::Run(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	FFaerieCraftingActionData SlotMemory;

	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		if (const FFaerieCraftingSlotsView CraftingSlots = Config->GetCraftingSlots();
			CraftingSlots.IsValid() && !Faerie::Generation::ValidateFilledSlots(UpgradeTarget.Slots, CraftingSlots.Get()))
		{
			return Runner->Fail();
		}
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionData>(SlotMemory);

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->Mutator.Get().GetRequiredAssets(RequiredAssets);

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
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulk::Execute, Runner));
}

void FFaerieItemUpgradeActionBulk::Execute(UFaerieCraftingRunner* Runner) const
{
	FFaerieCraftingActionData& RequestData = Runner->RequestStorage.GetMutable<FFaerieCraftingActionData>();

	// @todo batching
	int32 Copies = 1;

	// Prepare ProcessStacks
	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		if (Config->ReleaseWhileOperating)
		{
			RequestData.ProcessStacks.Add(UpgradeTarget.ItemProxy->Release(Copies));
		}
		else
		{
			RequestData.ProcessStacks.Add(FFaerieItemStack(UpgradeTarget.ItemProxy.GetItemObject(), Copies));
		}
	}

	FFaerieItemMutatorContext Context;
	Context.Squirrel = Squirrel.Get();

	if (!Faerie::Generation::ApplyConfigToProcessStacks(RequestData.ProcessStacks, Config, &Context))
	{
		return Runner->Fail();
	}

	if (RunConsumeStep)
	{
		if (Config->Implements<UFaerieItemSlotInterface>())
		{
			if (const FFaerieCraftingSlotsView SlotsView = Faerie::Generation::GetCraftingSlots(Config);
				SlotsView.IsValid())
			{
				for (auto&& UpgradeTarget : UpgradeTargets)
				{
					Faerie::Generation::ConsumeSlotCosts(UpgradeTarget.Slots, SlotsView.Get());
				}
			}
		}
	}

	Runner->Complete();
}