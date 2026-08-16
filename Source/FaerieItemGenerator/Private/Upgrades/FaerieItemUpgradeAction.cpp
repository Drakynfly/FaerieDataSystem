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

namespace CommonErrors
{
	static const FText CannotApplyFailure = INVTEXT("Upgrade Action failure: Cannot Apply Upgrade config to item");
	static const FText CannotPayCost = INVTEXT("Upgrade Action failure: Cannot pay cost");
	static const FText ExecutionFailure = INVTEXT("Upgrade Action failure: Upgrade execution failed");
}

void FFaerieItemUpgradeAction::Run(const Faerie::Generation::FActionExecution& Execution)
{
	if (!IsValid(ItemProxy.GetProxyObject()))
	{
		return Fail(Execution, this, INVTEXT("Invalid Proxy"));
	}

	if (!IsValid(Config))
	{
		return Fail(Execution, this, INVTEXT("Invalid Config"));
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
		return Execute(Execution);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsInGameWorld forces this action to be ran in the editor synchronously
	if (!Execution.IsInGameWorld())
	{
		// Immediately load all objects and continue.
		for (const FSoftObjectPath& Object : ObjectsToLoad)
		{
			Object.TryLoad();
		}

		return Execute(Execution);
	}

	// Suspend generation to async load drop assets, then continue
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeAction::Execute, Execution));
}

void FFaerieItemUpgradeAction::Execute(const Faerie::Generation::FActionExecution Execution)
{
	// @todo batching
	int32 Copies = 1;

	if (!Config->CanApplyUpgrade(Execution.EntityManager, ItemProxy))
	{
		return Fail(Execution, this, CommonErrors::CannotApplyFailure);
	}

	if (!Config->CanPayCost(Execution.EntityManager, Slots, ItemProxy))
	{
		return Fail(Execution, this, CommonErrors::CannotPayCost);
	}

	ActionData.Stacks.Add( FFaerieUnownedItemStack(ItemProxy.GetItemInstance().GetValue(), Copies));

	if (!Config->ApplyUpgrade(Execution.EntityManager, ActionData, Execution.Squirrel.Get()))
	{
		return Fail(Execution, this, CommonErrors::ExecutionFailure);
	}

	if (RunConsumeStep)
	{
		Config->PayCost(Execution.EntityManager, Slots, ItemProxy);
	}

	Complete(Execution, this, FText::GetEmpty());
}

void FFaerieItemUpgradeActionBulkNoPayment::Run(const Faerie::Generation::FActionExecution& Execution)
{
	if (!IsValid(Config))
	{
		return Fail(Execution, this, INVTEXT("Invalid Config"));
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
		return Execute(Execution);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsInGameWorld forces this action to be ran in the editor synchronously
	if (!Execution.IsInGameWorld())
	{
		// Immediately load all objects and continue.
		for (const FSoftObjectPath& Object : ObjectsToLoad)
		{
			Object.TryLoad();
		}

		return Execute(Execution);
	}

	// Suspend generation to async load drop assets, then continue
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulkNoPayment::Execute, Execution));
}

void FFaerieItemUpgradeActionBulkNoPayment::Execute(const Faerie::Generation::FActionExecution Execution)
{
	// Prepare Stacks
	ActionData.Stacks = UpgradeTargets;

	if (!Config->ApplyUpgrade(Execution.EntityManager, ActionData, Execution.Squirrel.Get()))
	{
		return Fail(Execution, this, CommonErrors::ExecutionFailure);
	}

	Complete(Execution, this, FText::GetEmpty());
}

void FFaerieItemUpgradeActionBulk::Run(const Faerie::Generation::FActionExecution& Execution)
{
	if (!IsValid(Config))
	{
		return Fail(Execution, this, INVTEXT("Invalid Config"));
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
		return Execute(Execution);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsInGameWorld forces this action to be run in the editor synchronously
	if (!Execution.IsInGameWorld())
	{
		// Immediately load all objects and continue.
		for (const FSoftObjectPath& Object : ObjectsToLoad)
		{
			Object.TryLoad();
		}

		return Execute(Execution);
	}

	// Suspend generation to async load drop assets, then continue
	RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		 FStreamableDelegate::CreateRaw(this, &FFaerieItemUpgradeActionBulk::Execute, Execution));
}

void FFaerieItemUpgradeActionBulk::Execute(const Faerie::Generation::FActionExecution Execution)
{
	// @todo batching
	int32 Copies = 1;

	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		if (!Config->CanPayCost(Execution.EntityManager, UpgradeTarget.Slots, UpgradeTarget.ItemProxy))
		{
			return Fail(Execution, this, CommonErrors::CannotPayCost);
		}
	}

	// Prepare Stacks
	for (auto&& UpgradeTarget : UpgradeTargets)
	{
		ActionData.Stacks.Emplace(UpgradeTarget.ItemProxy.GetItemInstance().GetValue(), Copies);
	}

	if (!Config->ApplyUpgrade(Execution.EntityManager, ActionData, Execution.Squirrel.Get()))
	{
		return Fail(Execution, this, CommonErrors::ExecutionFailure);
	}

	if (RunConsumeStep)
	{
		for (auto&& UpgradeTarget : UpgradeTargets)
		{
			Config->PayCost(Execution.EntityManager, UpgradeTarget.Slots, UpgradeTarget.ItemProxy);
		}
	}

	Complete(Execution, this, FText::GetEmpty());
}