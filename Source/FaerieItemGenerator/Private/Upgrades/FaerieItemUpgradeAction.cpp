// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeAction.h"
#include "Upgrades/FaerieItemUpgradeConfig.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemMutator.h"
#include "FaerieItemSlotInterface.h"
#include "ItemCraftingRunner.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeAction)

void FFaerieItemUpgradeAction::Run(TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	if (!IsValid(ItemProxy.GetObject()))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: ItemProxy is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->GetRequiredAssets(RequiredAssets);

	ObjectsToLoad.Reserve(RequiredAssets.Num());
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
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeAction::Execute, Runner));
}

void FFaerieItemUpgradeAction::Execute(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// @todo batching
	int32 Copies = 1;

	if (!Config->CanPayCost(Slots, FFaerieItemStackView(ItemProxy)))
	{
		return Fail(Runner);
	}

	if (Config->ReleaseWhileOperating)
	{
		ActionData.Stacks.Add(ItemProxy->Release(Copies));
	}
	else
	{
		ActionData.Stacks.Add(FFaerieItemStack(ItemProxy.GetItemObject(), Copies));
	}

	if (!Config->ApplyUpgrade(ActionData, Squirrel.Get()))
	{
		return Fail(Runner);
	}

	if (RunConsumeStep)
	{
		Config->PayCost(Slots, FFaerieItemStackView(ItemProxy));
	}

	Complete(Runner);
}

void FFaerieItemUpgradeActionBulkNoPayment::Run(TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->GetRequiredAssets(RequiredAssets);

	ObjectsToLoad.Reserve(RequiredAssets.Num());
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
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulkNoPayment::Execute, Runner));
}

void FFaerieItemUpgradeActionBulkNoPayment::Execute(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// Prepare Stacks
	ActionData.Stacks = UpgradeTargets;

	if (!Config->ApplyUpgrade(ActionData, Squirrel.Get()))
	{
		return Fail(Runner);
	}

	Complete(Runner);
}

void FFaerieItemUpgradeActionBulk::Run(TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	Config->GetRequiredAssets(RequiredAssets);

	ObjectsToLoad.Reserve(RequiredAssets.Num());
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

	// The check for IsGameWorld forces this action to be run in the editor synchronously
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
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulk::Execute, Runner));
}

void FFaerieItemUpgradeActionBulk::Execute(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// @todo batching
	int32 Copies = 1;

	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		if (!Config->CanPayCost(UpgradeTarget.Slots, FFaerieItemStackView(UpgradeTarget.ItemProxy)))
		{
			return Fail(Runner);
		}
	}

	// Prepare Stacks
	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		if (Config->ReleaseWhileOperating)
		{
			ActionData.Stacks.Add(UpgradeTarget.ItemProxy->Release(Copies));
		}
		else
		{
			ActionData.Stacks.Add(FFaerieItemStack(UpgradeTarget.ItemProxy.GetItemObject(), Copies));
		}
	}

	FFaerieItemMutatorContext Context;
	Context.Squirrel = Squirrel.Get();

	if (!Config->ApplyUpgrade(ActionData, Squirrel.Get()))
	{
		return Fail(Runner);
	}

	if (RunConsumeStep)
	{
		for (auto&& UpgradeTarget : UpgradeTargets)
		{
			Config->PayCost(UpgradeTarget.Slots, FFaerieItemStackView(UpgradeTarget.ItemProxy));
		}
	}

	Complete(Runner);
}