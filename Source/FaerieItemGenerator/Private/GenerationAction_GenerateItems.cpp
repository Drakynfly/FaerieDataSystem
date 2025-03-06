// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GenerationAction_GenerateItems.h"

#include "FaerieItem.h"
#include "FaerieItemDataProxy.h"
#include "ItemGeneratorConfig.h"
#include "ItemInstancingContext_Crafting.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GenerationAction_GenerateItems)

#define LOCTEXT_NAMESPACE "GenerationAction_GenerateItems"

void UGenerationAction_GenerateItems::Configure(FActionArgs& Args)
{
	check(!Args.Drivers.IsEmpty());

	for (auto&& Driver : Args.Drivers)
	{
		if (!IsValid(Driver)) continue;

		if (FPendingItemGeneration PendingEntry = Driver->Resolve();
			PendingEntry.IsValid())
		{
			PendingGenerations.Add(PendingEntry);
		}
	}

	Super::Configure(Args);
}

TArray<FSoftObjectPath> UGenerationAction_GenerateItems::GetAssetsToLoad() const
{
	TArray<FSoftObjectPath> ObjectsToLoad;

	for (const FPendingItemGeneration& PendingGeneration : PendingGenerations)
	{
		if (PendingGeneration.Drop.Asset.Object.IsPending())
		{
			ObjectsToLoad.Add(PendingGeneration.Drop.Asset.Object.ToSoftObjectPath());
		}
	}

	return ObjectsToLoad;
}

void UGenerationAction_GenerateItems::Run()
{
	if (PendingGenerations.IsEmpty())
	{
		return Fail();
	}

 	UItemInstancingContext_Crafting* Context = NewObject<UItemInstancingContext_Crafting>(this);

	for (auto&& Generation : PendingGenerations)
	{
		if (!Generation.IsValid())
		{
			UE_LOG(LogItemGenConfig, Warning, TEXT("--- Invalid generation!"));
			continue;
		}

		// Set the squirrel used for this iteration
		Context->Squirrel = Generation.Squirrel;

		// Generate individual mutable entries when mutable, as each may be unique.
		if (Cast<IFaerieItemSource>(Generation.Drop.Asset.Object.Get())->CanBeMutable())
		{
			for (int32 i = 0; i < Generation.Count; ++i)
			{
				FFaerieItemStack NewStack;
				NewStack.Item = Generation.Drop.Resolve(Context);
				NewStack.Copies = 1;

				if (IsValid(NewStack.Item))
				{
					ProcessStacks.Add(NewStack);
				}
				else
				{
					UE_LOG(LogGenerationAction, Error, TEXT("FTableDrop::Resolve returned a bad instance! Crafting likely failed"))
				}
			}
		}
		// Generate a single entry stack when immutable, as there is no chance of uniqueness.
		else
		{
			FFaerieItemStack NewStack;
			NewStack.Item = Generation.Drop.Resolve(Context);
			NewStack.Copies = Generation.Count;

			if (IsValid(NewStack.Item))
			{
				ProcessStacks.Add(NewStack);
			}
		}
	}

	if (!ProcessStacks.IsEmpty())
	{
		UE_LOG(LogItemGenConfig, Log, TEXT("--- Generation success. Created '%i' stack(s)."), ProcessStacks.Num());
		Complete();
	}
	else
	{
		UE_LOG(LogItemGenConfig, Error, TEXT("--- Generation failed to create any entries. Nothing will be returned."));
		return Fail();
	}
}

#undef LOCTEXT_NAMESPACE