// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/RunMutatorRequest.h"

#include "FaerieItemGenerationLog.h"
#include "FaerieItemMutator.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RunMutatorRequest)

void FFaerieRunMutatorRequest::Run(UFaerieCraftingRunner* Runner) const
{
	if (ItemStacks.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: ItemStacks is empty!"), __FUNCTION__);
		return Runner->Fail();
	}

	const FStructView MutatorView = Mutator.View;
	if (!MutatorView.IsValid() ||
		!MutatorView.GetScriptStruct()->IsChildOf<FFaerieItemMutator>())
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Mutator is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	TArray<FSoftObjectPath> ObjectsToLoad;

	// Preload any assets that the Mutator wants loaded
	TArray<TSoftObjectPtr<UObject>> RequiredAssets;
	MutatorView.Get<FFaerieItemMutator>().GetRequiredAssets(RequiredAssets);

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
		 FStreamableDelegate::CreateRaw(this, &FFaerieRunMutatorRequest::Execute, Runner));
}

void FFaerieRunMutatorRequest::Execute(UFaerieCraftingRunner* Runner) const
{
	Runner->RequestStorage.InitializeAs(FFaerieCraftingActionData());
	FFaerieCraftingActionData& OutData = Runner->RequestStorage.GetMutable<FFaerieCraftingActionData>();

	const FFaerieItemMutator& MutatorStruct = Mutator.View.Get<FFaerieItemMutator>();
	USquirrel* SquirrelPin = Squirrel.Get();

	OutData.ProcessStacks = ItemStacks;
	for (auto&& ItemStack : OutData.ProcessStacks)
	{
		if (!ItemStack.IsValid())
		{
			continue;
		}

		MutatorStruct.Apply(ItemStack, SquirrelPin);
	}

	Runner->Complete();
}