// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Generation/FaerieItemGenerationAction.h"
#include "Generation/FaerieItemGenerationConfig.h"

#include "EntityManagerHelpers.h"

#include "FaerieItemGenerationLog.h"
#include "FaerieItemPool.h"
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
	void ResolveGeneration(const FPendingTableDrop& PendingDrop, const FFaerieItemInstancingContext_Crafting& Context, FFaerieCraftingActionData& Data)
	{
		const UObject* AssetObject = PendingDrop.Drop->Asset.Object.Get();
		if (!AssetObject)
		{
			AssetObject = PendingDrop.Drop->Asset.Object.LoadSynchronous();
			check(AssetObject);
		}

		// Generate individual mutable entries when mutable, as each may be unique.
		if (const IFaerieItemSource* SourceObject = Cast<IFaerieItemSource>(AssetObject);
			SourceObject->CanBeMutable())
		{
			for (int32 i = 0; i < PendingDrop.Count; ++i)
			{
				if (auto NewStack = PendingDrop.Drop->Resolve(Context);
					NewStack.IsValid())
				{
					Data.Stacks.Emplace(NewStack.WithInitialization());
				}
				else
				{
					UE_LOGF(LogItemGeneration, Error, "FFaerieTableDrop::Resolve returned an invalid item! Generation failed")
				}
			}
		}
		// Generate a single entry stack when immutable, as there is no chance of uniqueness.
		else
		{
			if (auto NewStack = PendingDrop.Drop->Resolve(Context);
				NewStack.IsValid())
			{
				FFaerieUnownedItemStack Value = NewStack.WithInitialization();
				Value.Copies *= PendingDrop.Count;
				Data.Stacks.Emplace(Value);
			}
		}
	}
}

void FFaerieItemGenerationActionSingle::Run(const Generation::FActionExecution& Execution)
{
	if (Source.Asset.Object.IsNull())
	{
		return Fail(Execution, this, INVTEXT("Invalid source asset"));
	}

	LoadCheck(nullptr, Execution);
}

void FFaerieItemGenerationActionSingle::LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, const Generation::FActionExecution& Execution)
{
	TArray<FSoftObjectPath> ObjectsToLoad;

	if (LoadHandle.IsValid())
	{
		LoadHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
			{
				LoadedAssets.Add(LoadedObject);
			});
	}

	{
		auto&& Obj = Source.Asset.Object;
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

		auto&& ResourceObj = Source.Asset.Object;
		if (ResourceObj.IsValid())
		{
			if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(ResourceObj.Get()))
			{
			}
		}
		if (ResourceObj.IsPending())
		{
			ObjectsToLoad.Add(ResourceObj.ToSoftObjectPath());
		}
	}

	if (ObjectsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 3.
		return Generate(Execution);
	}

	UE_LOGF(LogItemGeneration, Log, "- Objects to load: %i", ObjectsToLoad.Num());

	// The check for IsInGameWorld forces this action to run in the editor synchronously
	if (Execution.IsInGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
			FStreamableDelegateWithHandle::CreateWeakLambda(Execution.Runner, [Execution, This = Handle](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				if (auto&& RunningAction = Execution.Runner->GetRunningAction(This);
					RunningAction.IsValid())
				{
					RunningAction.Get<FFaerieItemGenerationActionSingle>().LoadCheck(InLoadHandle, Execution);
				}
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
        LoadCheck(UAssetManager::GetStreamableManager().RequestSyncLoad(ObjectsToLoad), Execution);
	}
}

void FFaerieItemGenerationActionSingle::Generate(const Generation::FActionExecution& Execution)
{
	// Step 3: Build a context, to use for the pending generation, and resolve it.

	FFaerieItemInstancingContext_Crafting Context;
	Context.EntityManager = Execution.EntityManager;
	Context.Squirrel = Execution.Squirrel.Get();

	if (!Source.IsValid())
	{
		return Fail(Execution, this, INVTEXT("PendingDrop is invalid. Nothing will be returned"));
	}

	Generation::ResolveGeneration({.Drop = &Source, .Count = 1}, Context, ActionData);


	if (Execution.IsInGameWorld())
	{
		check(Execution.EntityManager)

		// Initialize all generated instances for runtime.
		for (auto&& Stack : ActionData.Stacks)
		{
			Stack.Instance.InitializeMassEntityIfInvalid(*Execution.EntityManager);
		}

		for (auto&& GeneratedChild : Context.GeneratedChildren)
		{
			GeneratedChild.Value.Instance.InitializeMassEntityIfInvalid(*Execution.EntityManager);
		}
	}

	// Step 4: Report result.

	if (!ActionData.Stacks.IsEmpty())
	{
		FText OutMessage;
#if WITH_EDITOR
		OutMessage = FText::AsCultureInvariant(FString::Printf(TEXT("Generation success. Created '%i' stack(s)."), ActionData.Stacks.Num()));
#endif
		return Complete(Execution, this, OutMessage);
	}
	else
	{
		return Fail(Execution, this, INVTEXT("Generation failed to create any entries. Nothing will be returned."));
	}
}

void FFaerieItemGenerationAction::Run(const Generation::FActionExecution& Execution)
{
	// Step 1: Validate parameters

	if (Drivers.IsEmpty())
	{
		return Fail(Execution, this, INVTEXT("Drivers are empty"));
	}

	for (auto&& Driver : Drivers)
	{
		if (Driver.IsNull())
		{
			return Fail(Execution, this, INVTEXT("Invalid driver"));
		}
	}

	TArray<FSoftObjectPath> ConfigsToLoad;

	for (auto&& Driver : Drivers)
	{
		if (Driver.IsValid())
		{
			UFaerieItemGenerationConfig* ConfigObj = Driver.Get();
			ConfigObj->Resolve(PendingDrops, Execution.Squirrel.Get());
		}
		else if (Driver.IsPending())
		{
			ConfigsToLoad.Add(Driver.ToSoftObjectPath());
		}
	}

	// Step 2: Load Configs

	if (ConfigsToLoad.IsEmpty())
	{
		// Nothing needs to load, go to Step 2.
		return LoadCheck(nullptr, Execution, 0);
	}

	UE_LOGF(LogItemGeneration, Log, "- Configs to load: %i", ConfigsToLoad.Num());

	// The check for IsInGameWorld forces this action to run in the editor synchronously
	if (Execution.IsInGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ConfigsToLoad,
			FStreamableDelegateWithHandle::CreateWeakLambda(Execution.Runner, [Execution, ThisHandle = Handle](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				if (auto&& RunningAction = Execution.Runner->GetRunningAction(ThisHandle);
					RunningAction.IsValid())
				{
					FFaerieItemGenerationAction& This = RunningAction.Get<FFaerieItemGenerationAction>();
					InLoadHandle->ForEachLoadedAsset([&This, Squirrel = Execution.Squirrel](const UObject* LoadedObject) mutable
						{
							// Keep the drivers alive while we are generating from them.
							This.LoadedAssets.Add(LoadedObject);
							if (const UFaerieItemGenerationConfig* Config = CastChecked<UFaerieItemGenerationConfig>(LoadedObject))
							{
								Config->Resolve(This.PendingDrops, Squirrel.Get());
							}
						});
					This.LoadCheck(nullptr, Execution, 0);
				}
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
		UAssetManager::GetStreamableManager().RequestSyncLoad(ConfigsToLoad)->ForEachLoadedAsset(
			[&](const UObject* LoadedObject)
			{
				LoadedAssets.Add(LoadedObject);
				if (const UFaerieItemGenerationConfig* Config = CastChecked<UFaerieItemGenerationConfig>(LoadedObject))
				{
					Config->Resolve(PendingDrops, Execution.Squirrel.Get());
				}
			});
        LoadCheck(nullptr, Execution, 0);
	}
}

void FFaerieItemGenerationAction::LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, const Generation::FActionExecution& Execution, const int32 CheckFromNum)
{
	check(this);

	TArray<FSoftObjectPath> ObjectsToLoad;

	if (LoadHandle.IsValid())
	{
		LoadHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
			{
				LoadedAssets.Add(LoadedObject);
			});
	}

	// Check if any new pending generations also need things loaded
	for (int32 i = CheckFromNum; i < PendingDrops.Num(); ++i)
	{
		auto&& Obj = PendingDrops[i].Drop->Asset.Object;
		if (Obj.IsValid())
		{
			if (const UFaerieItemPool* Pool = Cast<UFaerieItemPool>(Obj.Get()))
			{
				// If a pool was loaded, and we are configured to expand pools, do that now.
				if (RecursivelyResolveTables)
				{
					int32 DropCount = PendingDrops[i].Count;

					// Remove this pool from the table.
					PendingDrops.RemoveAtSwap(i, EAllowShrinking::No);
					i--;

					for (const FFaerieWeightedDrop& PoolDrop : Pool->ViewDropPool())
					{
						PendingDrops.Emplace(&PoolDrop.Drop, DropCount);
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
		return Generate(Execution);
	}

	UE_LOGF(LogItemGeneration, Log, "- Objects to load: %i", ObjectsToLoad.Num());
	const int32 CurrentPendingNum = PendingDrops.Num();

	// The check for IsInGameWorld forces this action to run in the editor synchronously
	if (Execution.IsInGameWorld())
	{
		// Suspend generation to async load drop assets, then continue
		RunningStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ObjectsToLoad,
			FStreamableDelegateWithHandle::CreateWeakLambda(Execution.Runner, [Execution, This = Handle, CurrentPendingNum](const TSharedPtr<FStreamableHandle>& InLoadHandle)
			{
				if (auto&& RunningAction = Execution.Runner->GetRunningAction(This);
					RunningAction.IsValid())
				{
					RunningAction.Get<FFaerieItemGenerationAction>().LoadCheck(InLoadHandle, Execution, CurrentPendingNum);
				}
			}));
	}
	else
	{
		// Load assets in-sync then keep searching
        LoadCheck(UAssetManager::GetStreamableManager().RequestSyncLoad(ObjectsToLoad), Execution, CurrentPendingNum);
	}
}

void FFaerieItemGenerationAction::Generate(const Generation::FActionExecution& Execution)
{
	// Step 3: Build a context, to use for each pending generation, and resolve them.

	FFaerieItemInstancingContext_Crafting Context;
	Context.EntityManager = Execution.EntityManager;
	Context.Squirrel = Execution.Squirrel.Get();

	for (const Generation::FPendingTableDrop& Generation : PendingDrops)
	{
		if (!Generation.IsValid())
		{
			UE_LOGF(LogItemGeneration, Warning, "--- Pending generation is invalid!");
			continue;
		}

		Generation::ResolveGeneration(Generation, Context, ActionData);
	}

	if (Execution.IsInGameWorld())
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();

		// Initialize all generated instances for runtime.
        for (auto&& Stack : ActionData.Stacks)
        {
        	Stack.Instance.InitializeMassEntityIfInvalid(EntityManager);
        }

        for (auto&& GeneratedChild : Context.GeneratedChildren)
        {
        	GeneratedChild.Value.Instance.InitializeMassEntityIfInvalid(EntityManager);
        }
	}


	// Step 4: Report result.

	if (ActionData.Stacks.IsEmpty())
	{
		return Fail(Execution, this, INVTEXT("Generation failed to create any entries. Nothing will be returned."));
	}

	FText OutMessage;
#if WITH_EDITOR
	OutMessage = FText::AsCultureInvariant(FString::Printf(TEXT("Generation success. Created '%i' stack(s)."), ActionData.Stacks.Num()));
#endif
	return Complete(Execution, this, OutMessage);
}

#undef LOCTEXT_NAMESPACE
