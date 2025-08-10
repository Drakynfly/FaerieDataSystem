// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Generation/FaerieItemGenerationRequest.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include "FaerieItem.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemPool.h"
#include "FaerieItemStack.h"
#include "ItemInstancingContext_Crafting.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemGenerationRequest)

#define LOCTEXT_NAMESPACE "FaerieItemGenerationRequest"

void FFaerieItemGenerationRequest::Run(UFaerieCraftingRunner* Runner) const
{
	if (Drivers.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Drivers are empty!"), __FUNCTION__);
		return Runner->Fail();
	}

	for (auto&& Driver : Drivers)
	{
		if (!Driver)
		{
			UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Driver is misconfigured!"), __FUNCTION__);
			return Runner->Fail();
		}
	}

	FFaerieItemGenerationRequestStorage Storage;

	for (auto&& Driver : Drivers)
	{
		if (!IsValid(Driver)) continue;

		Driver->Resolve(Storage.PendingGenerations, Squirrel.Get());
	}

	Runner->RequestStorage.InitializeAs<FFaerieItemGenerationRequestStorage>(Storage);

	TArray<FSoftObjectPath> ObjectsToLoad;

	for (const Faerie::FPendingItemGeneration& PendingGeneration : Storage.PendingGenerations)
	{
		if (PendingGeneration.Drop->Asset.Object.IsPending())
		{
			ObjectsToLoad.Add(PendingGeneration.Drop->Asset.Object.ToSoftObjectPath());
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing to wait for, Generate now!
		return Generate(Runner);
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

		return Generate(Runner);
	}

	// Suspend generation to async load drop assets, then continue afterwards.
	Runner->RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
		FStreamableDelegateWithHandle::CreateRaw(this, &FFaerieItemGenerationRequest::LoadCheck, Runner));
}

void FFaerieItemGenerationRequest::Generate(UFaerieCraftingRunner* Runner) const
{
	FFaerieItemGenerationRequestStorage& Storage = Runner->RequestStorage.GetMutable<FFaerieItemGenerationRequestStorage>();

	if (Storage.PendingGenerations.IsEmpty())
	{
		return Runner->Fail();
	}

	FFaerieItemInstancingContext_Crafting Context;

	// Set the squirrel used
	Context.Squirrel = Squirrel.Get();

	for (auto&& Generation : Storage.PendingGenerations)
	{
		if (!Generation.IsValid())
		{
			UE_LOG(LogItemGeneration, Warning, TEXT("--- Invalid generation!"));
			continue;
		}

		ResolveGeneration(Storage, Generation, Context);
	}

	if (!Storage.ProcessStacks.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Log, TEXT("--- Generation success. Created '%i' stack(s)."), Storage.ProcessStacks.Num());
		Runner->Complete();
	}
	else
	{
		UE_LOG(LogItemGeneration, Error, TEXT("--- Generation failed to create any entries. Nothing will be returned."));
		return Runner->Fail();
	}
}

void FFaerieItemGenerationRequest::LoadCheck(TSharedPtr<FStreamableHandle> Handle, UFaerieCraftingRunner* Runner) const
{
	TArray<UObject*> LoadedObjects;
	Handle->GetLoadedAssets(LoadedObjects);

	TArray<FSoftObjectPath> ObjectsToLoad;

	for (UObject* LoadedObject : LoadedObjects)
	{
		if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(LoadedObject);
			IsValid(Pool) && RecursivelyResolveTables)
		{
			for (auto&& Drop : Pool->ViewDropPool())
			{
				ObjectsToLoad.Add(Drop.Drop.Asset.Object.ToSoftObjectPath());
			}
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing to wait for, run now!
		return Generate(Runner);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsGameWorld forces this action to be ran in the editor synchronously
	if (Runner->GetWorld()->IsGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		Runner->RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
			FStreamableDelegateWithHandle::CreateRaw(this, &FFaerieItemGenerationRequest::LoadCheck, Runner));
		return;
	}

	// Immediately load all objects and continue.
	for (const FSoftObjectPath& Object : ObjectsToLoad)
	{
		Object.TryLoad();
	}

	return Generate(Runner);
}

void FFaerieItemGenerationRequest::ResolveGeneration(FFaerieItemGenerationRequestStorage& Storage, const Faerie::FPendingItemGeneration& Generation, const FFaerieItemInstancingContext_Crafting& Context) const
{
	const UObject* SourceObject = Generation.Drop->Asset.Object.Get();
	if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(SourceObject);
		IsValid(Pool) && RecursivelyResolveTables)
	{
		for (auto&& WeightedDrop : Pool->ViewDropPool())
		{
			Faerie::FPendingItemGeneration SubGeneration;
			SubGeneration.Drop = &WeightedDrop.Drop;
			SubGeneration.Count = Generation.Count;
			ResolveGeneration(Storage, SubGeneration, Context);
		}
		return;
	}

	// Generate individual mutable entries when mutable, as each may be unique.
	if (Cast<IFaerieItemSource>(SourceObject)->CanBeMutable())
	{
		for (int32 i = 0; i < Generation.Count; ++i)
		{
			if (const UFaerieItem* Item = Generation.Drop->Resolve(Context);
				IsValid(Item))
			{
				Storage.ProcessStacks.Emplace(Item, 1);
			}
			else
			{
				UE_LOG(LogItemGeneration, Error, TEXT("FTableDrop::Resolve returned an invalid item! Generation failed"))
			}
		}
	}
	// Generate a single entry stack when immutable, as there is no chance of uniqueness.
	else
	{
		if (const UFaerieItem* Item = Generation.Drop->Resolve(Context);
			IsValid(Item))
		{
			Storage.ProcessStacks.Emplace(Item, Generation.Count);
		}
	}
}

#undef LOCTEXT_NAMESPACE
