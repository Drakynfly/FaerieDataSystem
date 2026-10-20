// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationActor.h"

#include "FaerieItem.h"
#include "EntityManagerHelpers.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemOwnership.h"
#include "MassCommandBuffer.h"

#include "Engine/Engine.h"

#include "MassReplication/FaerieMassReplicationSubsystem.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassReplicationActor)

using namespace Faerie;

void FFaerieMassReplicatedEntity::PreReplicatedRemove(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieInventory, Verbose, "FFaerieMassReplicatedEntity::PreReplicatedRemove")
	InArraySerializer.Owner->Client_RemoveEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedAdd(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieInventory, Verbose, "FFaerieMassReplicatedEntity::PostReplicatedAdd")
	InArraySerializer.Owner->Client_AddEntity(*this);
}

void FFaerieMassReplicatedEntity::PostReplicatedChange(const FFaerieMassReplicatedEntities& InArraySerializer)
{
	UE_LOGF(LogFaerieInventory, Verbose, "FFaerieMassReplicatedEntity::PostReplicatedChange")
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

void AFaerieMassReplicationActor::BeginPlay()
{
	Super::BeginPlay();

	GetEntityManager();

	UFaerieMassReplicationSubsystem* Replication = GetWorld()->GetSubsystem<UFaerieMassReplicationSubsystem>();
	Replication->ConfirmReplicationActor(this);
}

void AFaerieMassReplicationActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (auto&& Entity : Metadata)
	{
		if (Entity.Value.AwaitingContainerAck)
		{
			CheckAck(Entity.Key, Entity.Value);
		}
	}
}

TOptional<FMassEntityHandle> AFaerieMassReplicationActor::ResolveNetId(const FFaerieNetItemID NetID) const
{
	if (const Container::FReplicatedItemMetadata* Element = Metadata.Find(NetID))
	{
		return Element->EntityHandle;
	}
	return NullOpt;
}

void AFaerieMassReplicationActor::Server_UpdateFragments(const FMassEntityHandle Item, const FFaerieNetItemID NetID, const TConstArrayView<FConstStructView> FragmentViews)
{
	// Try to update existing entry.
	for (FFaerieMassReplicatedEntity& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.NetID != NetID) continue;

		for (const FConstStructView& FragmentView : FragmentViews)
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
	Metadata.FindOrAdd(NetID).EntityHandle = Item;
	NewEntry.NetID = NetID;
	for (const FConstStructView& FragmentView : FragmentViews)
	{
		NewEntry.Fragments.Emplace(FragmentView);
	}
	ReplicatedEntities.MarkItemDirty(NewEntry);
}

void AFaerieMassReplicationActor::Server_RemoveFragments(const FFaerieNetItemID NetID, const TConstArrayView<const UScriptStruct*> FragmentTypes)
{
	for (FFaerieMassReplicatedEntity& ReplicatedEntity : ReplicatedEntities.Entries)
	{
		if (ReplicatedEntity.NetID != NetID) continue;

		for (auto It = ReplicatedEntity.Fragments.CreateIterator(); It; ++It)
		{
			if (FragmentTypes.Contains(It->GetScriptStruct()))
			{
				It.RemoveCurrentSwap();
			}
		}
		ReplicatedEntities.MarkItemDirty(ReplicatedEntity);
		break;
	}
}

void AFaerieMassReplicationActor::Server_RemoveEntities(const TConstArrayView<FFaerieNetItemID> NetIDs)
{
	bool RemovedEntity = false;
	for (auto&& It = ReplicatedEntities.Entries.CreateIterator(); It; ++It)
	{
		if (NetIDs.Contains(It->NetID))
		{
			Metadata.Remove(It->NetID);
			It.RemoveCurrent();
			RemovedEntity = true;
		}
	}

	if (RemovedEntity)
	{
		ReplicatedEntities.MarkArrayDirty();
	}
}

void AFaerieMassReplicationActor::Client_AddEntity(FFaerieMassReplicatedEntity& Entity)
{
	check(Entity.NetID.IsSet());

	FMassEntityManager& EntityManager = GetEntityManager();

	const FMassEntityHandle EntityHandle = ItemData::InitializeItemEntity(EntityManager, nullptr, FMassTagBitSet(), Entity.Fragments);
	check(EntityManager.IsEntityBuilt(EntityHandle));

	Container::FReplicatedItemMetadata& Element = Metadata.FindOrAdd(Entity.NetID);
	Element.EntityHandle = EntityHandle;
	Element.AwaitingContainerAck = true;

	// @todo do we really event need to emit events when adding a new entity? are there ever view models already made? technically storages can emit proxies before the entries replicate, so maybe.
	for (FInstancedStruct& Fragment : Entity.Fragments)
	{
		// Signal that all data changed, because we don't know what the server updated.
		ItemData::OnItemFragmentEdited(EntityManager, EntityHandle, Fragment.GetScriptStruct(), ItemData::AllFields);
	}

	CheckAck(Entity.NetID, Element);
}

void AFaerieMassReplicationActor::Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity)
{
	// Local item has already been initialized, apply delta.
	FMassEntityManager& EntityManager = GetEntityManager();

	if (const Container::FReplicatedItemMetadata* Element = Metadata.Find(Entity.NetID))
	{
		ItemData::UpdateFragments(EntityManager, Element->EntityHandle, Entity.Fragments, true);
	}
}

void AFaerieMassReplicationActor::Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity)
{
	FMassEntityManager& EntityManager = GetEntityManager();
	if (const Container::FReplicatedItemMetadata* Element = Metadata.Find(Entity.NetID))
	{
		if (EntityManager.IsEntityValid(Element->EntityHandle))
		{
			EntityManager.DestroyEntity(Element->EntityHandle);
		}
		Metadata.Remove(Entity.NetID);
	}
}

FMassEntityManager& AFaerieMassReplicationActor::GetEntityManager()
{
	if (!EntityManagerPtr.IsValid())
	{
		EntityManagerPtr = ItemData::GetFaerieEntityManagerChecked(GetWorld()).AsShared();
	}
	return *EntityManagerPtr.Get();
}

void AFaerieMassReplicationActor::CheckAck(const FFaerieNetItemID NetID, Container::FReplicatedItemMetadata& Entity)
{
	FMassEntityManager& EntityManager = GetEntityManager();

	// See if we can resolve a valid Container pointer from this item.
	// @todo GetItemOwner will spam invalid owner here, disable that warning, as we expect it, its just spam
	if (UFaerieItemContainerBase* Container = Container::GetItemOwner(EntityManager, Entity.EntityHandle))
	{
		// Inform the container object the network has generated a paired entity handle for its NetID.
		if (Container->AckItemCreatedByNetwork(EntityManager, Entity.EntityHandle, NetID))
		{
			// Item was successfully bound to owner.
			Entity.AwaitingContainerAck = false;
		}
	}
}
