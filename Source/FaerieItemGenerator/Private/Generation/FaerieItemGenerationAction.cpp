// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Generation/FaerieItemGenerationAction.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include "FaerieItemGenerationLog.h"
#include "FaerieItemPool.h"
#include "FaerieItemStack.h"
#include "ItemCraftingRunner.h"
#include "ItemInstancingContext_Crafting.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemGenerationAction)

#define LOCTEXT_NAMESPACE "FaerieItemGenerationAction"

using namespace Faerie;

namespace Faerie::Generation
{
	void ResolveGeneration(const FPendingTableDrop& Generation, const FFaerieItemInstancingContext_Crafting& Context, FFaerieCraftingActionData& Data)
	{
		const UObject* SourceObject = Generation.Drop->Asset.Object.Get();

		// Generate individual mutable entries when mutable, as each may be unique.
		if (Cast<IFaerieItemSource>(SourceObject)->CanBeMutable())
		{
			for (int32 i = 0; i < Generation.Count; ++i)
			{
				if (auto NewStack = Generation.Drop->Resolve(Context);
					NewStack.IsSet())
				{
					Data.Stacks.Emplace(NewStack.GetValue());
				}
				else
				{
					UE_LOG(LogItemGeneration, Error, TEXT("FFaerieTableDrop::Resolve returned an invalid item! Generation failed"))
				}
			}
		}
		// Generate a single entry stack when immutable, as there is no chance of uniqueness.
		else
		{
			if (auto NewStack = Generation.Drop->Resolve(Context);
				NewStack.IsSet())
			{
				FFaerieItemStack Value = NewStack.GetValue();
				Value.Copies *= Generation.Count;
				Data.Stacks.Emplace(Value);
			}
		}
	}
}

void FFaerieItemGenerationActionSingle::Run(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	if (Source.Asset.Object.IsNull())
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Source is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	LoadCheck(nullptr, Runner);
}

void FFaerieItemGenerationActionSingle::LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	TArray<FSoftObjectPath> ObjectsToLoad;

	/*
	if (LoadHandle.IsValid())
	{
		LoadHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
			{
			});
	}
	*/

	{
		const TSoftObjectPtr<UObject>& Obj = Source.Asset.Object;
		if (Obj.IsValid())
		{
			if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(Obj.Get()))
			{
			}
		}
		if (Obj.IsPending())
		{
			ObjectsToLoad.Add(Obj.ToSoftObjectPath());
		}
	}
	for (auto&& ResourceSlot : Source.StaticResourceSlots)
	{
		const FFaerieTableDrop& SlotSource = ResourceSlot.Value.Get();

		if (SlotSource.Asset.Object.IsValid())
		{
			if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(SlotSource.Asset.Object.Get()))
			{
			}
		}
		if (SlotSource.Asset.Object.IsPending())
		{
			ObjectsToLoad.Add(SlotSource.Asset.Object.ToSoftObjectPath());
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 3.
		return Generate(Runner);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());

	// The check for IsGameWorld forces this action to run in the editor synchronously
	if (Runner->GetWorld()->IsGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
			FStreamableDelegateWithHandle::CreateLambda([Runner, This = Handle](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				FFaerieItemGenerationActionSingle& Action = Runner->GetRunningAction(This)->GetMutable<FFaerieItemGenerationActionSingle>();
				Action.LoadCheck(InLoadHandle, Runner);
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
        LoadCheck(UAssetManager::GetStreamableManager().RequestSyncLoad(ObjectsToLoad), Runner);
	}
}

void FFaerieItemGenerationActionSingle::Generate(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// Step 3: Build a context, to use for the pending generation, and resolve it.

	FFaerieItemInstancingContext_Crafting Context;
	Context.Squirrel = Squirrel.Get();

	const Generation::FPendingTableDrop Drop { &Source, 1 };
	Generation::ResolveGeneration(Drop, Context, ActionData);

	// Step 4: Report result.

	if (!ActionData.Stacks.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Log, TEXT("--- Generation success. Created '%i' stack(s)."), ActionData.Stacks.Num());
		return Complete(Runner);
	}
	else
	{
		UE_LOG(LogItemGeneration, Error, TEXT("--- Generation failed to create any entries. Nothing will be returned."));
		return Fail(Runner);
	}
}

void FFaerieItemGenerationAction::Run(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// Step 1: Validate parameters

	if (Drivers.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Drivers are empty!"), __FUNCTION__);
		return Fail(Runner);
	}

	for (auto&& Driver : Drivers)
	{
		if (Driver.IsNull())
		{
			UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Driver is invalid!"), __FUNCTION__);
			return Fail(Runner);
		}
	}

	// Step 2: Repeat LoadCheck while finding objects to load.
	LoadDrivers(Runner);
}

void FFaerieItemGenerationAction::LoadDrivers(TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	TArray<FSoftObjectPath> ConfigsToLoad;

	for (auto&& Driver : Drivers)
	{
		if (Driver.IsValid())
		{
			UFaerieItemGenerationConfig* ConfigObj = Driver.Get();
			ConfigObj->Resolve(PendingGenerations, Squirrel.Get());
		}
		else if (Driver.IsPending())
		{
			ConfigsToLoad.Add(Driver.ToSoftObjectPath());
		}
	}

	if (ConfigsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 2.
		return LoadCheck(nullptr, Runner, 0);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Configs to load: %i"), ConfigsToLoad.Num());

	// The check for IsGameWorld forces this action to run in the editor synchronously
	if (Runner->GetWorld()->IsGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ConfigsToLoad,
			FStreamableDelegateWithHandle::CreateLambda([Runner, ThisHandle = Handle](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				FFaerieItemGenerationAction& This = Runner->GetRunningAction(ThisHandle)->GetMutable<FFaerieItemGenerationAction>();
				InLoadHandle->ForEachLoadedAsset([This](const UObject* LoadedObject) mutable
					{
						if (const UFaerieItemGenerationConfig* Config = Cast<UFaerieItemGenerationConfig>(LoadedObject))
						{
							Config->Resolve(This.PendingGenerations, This.Squirrel.Get());
						}
					});
				This.LoadCheck(nullptr, Runner, 0);
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
		UAssetManager::GetStreamableManager().RequestSyncLoad(ConfigsToLoad)->ForEachLoadedAsset(
			[&](const UObject* LoadedObject)
			{
				if (const UFaerieItemGenerationConfig* Config = Cast<UFaerieItemGenerationConfig>(LoadedObject))
				{
					Config->Resolve(PendingGenerations, Squirrel.Get());
				}
			});
        LoadCheck(nullptr, Runner, 0);
	}
}

void FFaerieItemGenerationAction::LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, TNotNull<UFaerieItemCraftingRunner*> Runner, const int32 CheckFromNum)
{
	TArray<FSoftObjectPath> ObjectsToLoad;

	/*
	if (LoadHandle.IsValid())
	{
		LoadHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
			{
			});
	}
	*/

	// Check if any new pending generations also need things loaded
	for (int32 i = CheckFromNum; i < PendingGenerations.Num(); ++i)
	{
		const TSoftObjectPtr<UObject>& Obj = PendingGenerations[i].Drop->Asset.Object;
		if (Obj.IsValid())
		{
			if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(Obj.Get()))
			{
				// If a pool was loaded, and we are configured to expand pools, do that now.
				if (RecursivelyResolveTables)
				{
					int32 DropCount = PendingGenerations[i].Count;

					// Remove this pool from the table.
					PendingGenerations.RemoveAtSwap(i, EAllowShrinking::No);
					i--;

					for (const FFaerieWeightedDrop& PoolDrop : Pool->ViewDropPool())
					{
						PendingGenerations.Emplace(&PoolDrop.Drop, DropCount);
					}
				}
			}
		}
		if (Obj.IsPending())
		{
			ObjectsToLoad.Add(Obj.ToSoftObjectPath());
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 3.
		return Generate(Runner);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("- Objects to load: %i"), ObjectsToLoad.Num());
	const int32 CurrentPendingNum = PendingGenerations.Num();

	// The check for IsGameWorld forces this action to run in the editor synchronously
	if (Runner->GetWorld()->IsGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
			FStreamableDelegateWithHandle::CreateLambda([Runner, This = Handle, CurrentPendingNum](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				FFaerieItemGenerationAction& Action = Runner->GetRunningAction(This)->GetMutable<FFaerieItemGenerationAction>();
				Action.LoadCheck(InLoadHandle, Runner, CurrentPendingNum);
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
        LoadCheck(UAssetManager::GetStreamableManager().RequestSyncLoad(ObjectsToLoad), Runner, CurrentPendingNum);
	}
}

void FFaerieItemGenerationAction::Generate(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	// Step 3: Build a context, to use for each pending generation, and resolve them.

	FFaerieItemInstancingContext_Crafting Context;
	Context.Squirrel = Squirrel.Get();

	for (auto&& Generation : PendingGenerations)
	{
		if (!Generation.IsValid())
		{
			UE_LOG(LogItemGeneration, Warning, TEXT("--- Invalid generation!"));
			continue;
		}

		ResolveGeneration(Generation, Context, ActionData);
	}

	// Step 4: Report result.

	if (!ActionData.Stacks.IsEmpty())
	{
		UE_LOG(LogItemGeneration, Log, TEXT("--- Generation success. Created '%i' stack(s)."), ActionData.Stacks.Num());
		return Complete(Runner);
	}
	else
	{
		UE_LOG(LogItemGeneration, Error, TEXT("--- Generation failed to create any entries. Nothing will be returned."));
		return Fail(Runner);
	}
}

#undef LOCTEXT_NAMESPACE
