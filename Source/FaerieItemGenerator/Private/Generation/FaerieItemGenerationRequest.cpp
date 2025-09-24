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
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemGenerationRequest)

#define LOCTEXT_NAMESPACE "FaerieItemGenerationRequest"

void FFaerieItemGenerationRequest::Run(UFaerieCraftingRunner* Runner) const
{
	// Step 0: Validate parameters

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

	// Step 1: Collect generations from Drivers, and collate into storage.

	FFaerieItemGenerationRequestStorage Storage;

	for (auto&& Driver : Drivers)
	{
		if (!IsValid(Driver)) continue;

		Driver->Resolve(Storage.PendingGenerations, Squirrel.Get());
	}

	if (Storage.PendingGenerations.IsEmpty())
	{
		return Runner->Fail();
	}

	Runner->RequestStorage.InitializeAs<FFaerieItemGenerationRequestStorage>(Storage);

	// Step 2: Load assets needed for Pending Generations
	LoadCheck(nullptr, Runner);
}

void FFaerieItemGenerationRequest::LoadCheck(TSharedPtr<FStreamableHandle> Handle, UFaerieCraftingRunner* Runner) const
{
	TArray<FSoftObjectPath> ObjectsToLoad;

	if (Handle.IsValid())
	{
		TArray<UObject*> LoadedObjects;

		Handle->GetLoadedAssets(LoadedObjects);

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
	}
	else
	{
		FFaerieItemGenerationRequestStorage& Storage = Runner->RequestStorage.GetMutable<FFaerieItemGenerationRequestStorage>();

		for (const Faerie::FPendingItemGeneration& PendingGeneration : Storage.PendingGenerations)
		{
			const TSoftObjectPtr<UObject>& Obj = PendingGeneration.Drop->Asset.Object;
			if (Obj.IsPending())
			{
				ObjectsToLoad.Add(Obj.ToSoftObjectPath());
			}
			else if (Obj.IsValid())
			{
				if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(Obj.Get());
					IsValid(Pool) && RecursivelyResolveTables)
				{
					for (auto&& Drop : Pool->ViewDropPool())
					{
						ObjectsToLoad.Add(Drop.Drop.Asset.Object.ToSoftObjectPath());
					}
				}
			}
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 3.
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

	// Done loading, go to Step 3.
	return Generate(Runner);
}

void FFaerieItemGenerationRequest::Generate(UFaerieCraftingRunner* Runner) const
{
	FFaerieItemGenerationRequestStorage& Storage = Runner->RequestStorage.GetMutable<FFaerieItemGenerationRequestStorage>();

	// Step 3: Build a context, to use for each pending generation, and resolve them.

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

	// Step 4: Report result.

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
