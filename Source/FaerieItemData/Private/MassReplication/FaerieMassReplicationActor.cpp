// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationActor.h"

#include "FaerieItem.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemDataLog.h"
#include "MassCommandBuffer.h"

#include "Engine/Engine.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassReplicationActor)

using namespace Faerie;

void FFaerieMassReplicatedEntity::PreReplicatedRemove(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieItemData, Verbose, "FFaerieMassReplicatedEntity::PreReplicatedRemove")
	InArraySerializer.Owner->Client_RemoveEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedAdd(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieItemData, Verbose, "FFaerieMassReplicatedEntity::PostReplicatedAdd")
	InArraySerializer.Owner->Client_AddEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedChange(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieItemData, Verbose, "FFaerieMassReplicatedEntity::PostReplicatedChange")
	InArraySerializer.Owner->Client_UpdateEntity(*this);
}

AFaerieMassReplicationActor::AFaerieMassReplicationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	ReplicatedEntities.Owner = this;
}

void AFaerieMassReplicationActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ReplicatedEntities);
}

void AFaerieMassReplicationActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Client-side check to fixup mass after receiving item pointers.
	if (GetNetMode() == NM_Client)
	{
		for (FFaerieMassReplicatedEntity& Entity : ReplicatedEntities.Entries)
		{
			Client_CheckItemPointer(Entity);
		}
	}
}

void AFaerieMassReplicationActor::Server_UpdateFragments(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews)
{
	// Try to update existing entry.
	for (FFaerieMassReplicatedEntity& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.EntityHandle != Item) continue;

		for (const TConstStructView<FFaerieMassFragment>& FragmentView : FragmentViews)
		{
			// Linear search. Probably will not have enough fragments in one item for this to be slow enough to attempt optimizing.
			if (FInstancedStruct* ReplicatedFragment = ReplicatedEntity.Fragments.FindByPredicate(
				[&FragmentView](const FInstancedStruct& Fragment)
				{
					return Fragment.GetScriptStruct() == FragmentView.GetScriptStruct();
				}))
			{
				// Found the entity, overwrite existing fragment
				*ReplicatedFragment = FragmentView;
			}
			else
			{
				// There was no existing fragment for this entity.
				ReplicatedEntity.Fragments.Emplace(FragmentView);
			}
		}

		ReplicatedEntities.MarkItemDirty(ReplicatedEntity);
		return;
	}

	// There was no existing entry for this entity.
	FFaerieMassReplicatedEntity& NewEntry = ReplicatedEntities.Entries.AddDefaulted_GetRef();
	NewEntry.EntityHandle = Item;
	for (const TConstStructView<FFaerieMassFragment>& FragmentView : FragmentViews)
	{
		NewEntry.Fragments.Emplace(FragmentView);
	}
	ReplicatedEntities.MarkItemDirty(NewEntry);
}

void AFaerieMassReplicationActor::Server_RemoveFragments(const FMassEntityManager& EntityManager, const FMassEntityHandle Item, const TConstArrayView<const UScriptStruct*> FragmentTypes)
{
	if (!EntityManager.IsEntityValid(Item))
	{
		UE_LOGF(LogFaerieItemData, Fatal, "Item created without entity handle. Item creation should always initialize itself with mass!")
		return;
	}

	for (FFaerieMassReplicatedEntity& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.EntityHandle != Item) continue;

		for (auto It = ReplicatedEntity.Fragments.CreateIterator(); It; ++It)
		{
			if (FragmentTypes.Contains(It->GetScriptStruct()))
			{
				It.RemoveCurrentSwap();
				ReplicatedEntities.MarkArrayDirty();
				return;
			}
		}
	}
}

void AFaerieMassReplicationActor::Server_RemoveEntities(const FMassEntityManager& EntityManager, const TConstArrayView<FMassEntityHandle> Items)
{
	for (auto&& It = ReplicatedEntities.Entries.CreateIterator(); It; ++It)
	{
		if (Items.Contains(It->EntityHandle))
		{
			It.RemoveCurrent();
			ReplicatedEntities.MarkArrayDirty();
			return;
		}
	}
}

void AFaerieMassReplicationActor::Client_AddEntity(FFaerieMassReplicatedEntity& Entity)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	// @todo do we really event need to emit events when adding a new entity? are there ever view models already made? technically storages can emit proxies before the entries replicate, so maybe.
	for (FInstancedStruct& Fragment : Entity.Fragments)
	{
		// Signal that all data changed, because we don't know what the server updated.
		FFaerieItemInstance::OnItemFragmentEdited(EntityManager, Entity.EntityHandle, Fragment.GetScriptStruct(), ItemData::AllFields);
	}

	Client_CheckItemPointer(Entity);
}

void AFaerieMassReplicationActor::Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	// Local item has already been initialized, apply delta.
	FFaerieItemInstance::UpdateFragments(EntityManager, Entity.EntityHandle, Entity.Fragments, true);
}

void AFaerieMassReplicationActor::Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	if (EntityManager.IsEntityValid(Entity.EntityHandle))
	{
		EntityManager.DestroyEntity(Entity.EntityHandle);
	}
}

void AFaerieMassReplicationActor::Client_CheckItemPointer(FFaerieMassReplicatedEntity& Entity)
{
	if (!Entity.HasImportedItemPointer && IsValid(Entity.ItemPointer))
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

		// If the entity manager stores info for this instance, push our item to it.
		if (EntityManager.IsEntityValid(Entity.EntityHandle))
		{
			EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
					[Handle = Entity.EntityHandle, Item = Entity.ItemPointer](FMassEntityManager& InEntityManager)
					{
						// All faerie item instances must have this fragment.
						FFaerieMassItemPointer& FragmentPtr = InEntityManager.GetFragmentDataChecked<FFaerieMassItemPointer>(Handle);
						FragmentPtr.Item = Item;
					});
		}
		Entity.HasImportedItemPointer = true;
	}
}
