// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieItemDataMassReplicators.h"
#include "MassReplication/FaerieMassReplicationSubsystem.h"

#include "FaerieContainerEvent.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemEvent.h"
#include "FaerieMassFragment.h"

#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataMassReplicators)

using namespace Faerie;

UFaerieItemDataCreationReplicator::UFaerieItemDataCreationReplicator()
  : EntityQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);

	// We broadcast events to the replication subsystem, so we need to run on the game thread.
	bRequiresGameThreadExecution = true;
}

void UFaerieItemDataCreationReplicator::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddSparseRequirement<Container::FNeedsReplicationInit>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FFaerieNetIDFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataCreationReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
	{
		ProcessEntities(InContext, ReplicationSubsystem);
	});
}

void UFaerieItemDataCreationReplicator::ProcessEntities(FMassExecutionContext& Context, UFaerieMassReplicationSubsystem& ReplicationSubsystem)
{
	const TConstArrayView<FFaerieNetIDFragment> NetIDs = Context.GetFragmentView<FFaerieNetIDFragment>();

	FMassEntityManager& Manager = Context.GetEntityManagerChecked();

	for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
	{
		// Build Item Instance from mass views.
		const FMassEntityHandle Entity = EntityIt.GetEntityHandle();
		const FFaerieNetIDFragment& NetID = NetIDs[EntityIt];

		// Create view for each dirty fragment.
		TArray<FConstStructView> FragmentViews;

		const FMassArchetypeHandle Archetype = Manager.GetArchetypeForEntity(Entity);
		Manager.ForEachArchetypeElementType(Archetype,
			[Entity, &Manager, &FragmentViews](const UScriptStruct* FragmentType)
			{
				if (const ItemData::FMassTypeTraitsInterface* Traits = ItemData::GetMassTraitsInterface(FragmentType))
				{
					if (!Traits->Replicated)
					{
						// Skip fragments that do not get replicated
						return true;
					}
				}

				if (const FConstStructView FragmentView = ItemData::GetEntityFragment(Manager, Entity, FragmentType);
					FragmentView.IsValid())
				{
					FragmentViews.Add(FragmentView);
				}
				return true;
			});

		// Push all new fragments into the replication subsystem.
		ReplicationSubsystem.Server_UpdateFragments(Entity, NetID.NetID, FragmentViews);

		// Replication init handled, remove tag.
		Context.Defer().RemoveTag<Container::FNeedsReplicationInit>(Entity);
	}
}

UFaerieItemDataDeletionReplicator::UFaerieItemDataDeletionReplicator()
  : EntityQuery(*this)
{
	// Observe destruction of entities with an item pointer.
	ObservedTypes = { FFaerieNetIDFragment::StaticStruct() };
	ObservedOperations = EMassObservedOperationFlags::Remove;

	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);

	// We broadcast events to the replication subsystem, so we need to run on the game thread.
	bRequiresGameThreadExecution = true;
}

void UFaerieItemDataDeletionReplicator::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FFaerieNetIDFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataDeletionReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
		{
			static_assert(sizeof(FFaerieNetIDFragment) == sizeof(FFaerieNetItemID));
			TConstArrayView<FFaerieNetIDFragment> Fragments = InContext.GetFragmentView<FFaerieNetIDFragment>();
			ReplicationSubsystem.Server_RemoveEntities(reinterpret_cast<TConstArrayView<FFaerieNetItemID>&>(Fragments));
		});
}

UFaerieItemDataChangeReplicator::UFaerieItemDataChangeReplicator()
  : EventQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);

	ExecutionOrder.ExecuteBefore.Add(ItemData::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieItemDataChangeReplicator::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<ItemData::FMutationEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	EventQuery.AddRequirement<ItemData::FMutationPayloadChangeList>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EventQuery.AddIndirectFragmentRequirement<FFaerieNetIDFragment>(EMassFragmentAccess::ReadOnly);
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataChangeReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EventQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
	{
		ProcessEntities(InContext, ReplicationSubsystem);
	});
}

void UFaerieItemDataChangeReplicator::ProcessEntities(FMassExecutionContext& Context, UFaerieMassReplicationSubsystem& ReplicationSubsystem)
{
	FMassEntityManager& Manager = Context.GetEntityManagerChecked();

	const TConstArrayView<ItemData::FMutationEvent> Events = Context.GetFragmentView<ItemData::FMutationEvent>();
	if (Events.IsEmpty()) return;

	for (int32 i = 0; i < Context.GetNumEntities(); ++i)
	{
		FMassEntityHandle EventEntity = Context.GetEntity(i);
		const ItemData::FMutationEvent& Event = Events[i];

		const FFaerieNetIDFragment* ItemNetIDPtr = Context.GetIndirectFragmentPtr<FFaerieNetIDFragment>(Event.ItemHandle);
		if (!ItemNetIDPtr)
		{
			// Cannot update ReplicationSubsystem without NetID
			continue;
		}

		if (Event.ChangeType == ItemData::FMutationPayloadChangeList::StaticStruct())
		{
			const ItemData::FMutationPayloadChangeList& ChangeList = Context.GetEntityManagerChecked().GetFragmentDataChecked<ItemData::FMutationPayloadChangeList>(EventEntity);

			if (Event.EventType == ItemData::Tags::FragmentAdd)
			{
				TArray<FConstStructView> FragmentViews;
				FragmentViews.Reserve(ChangeList.ChangeTypes.CountStoredTypes());

				for (auto It = ChangeList.ChangeTypes.GetIndexIterator(); It; ++It)
				{
					const UScriptStruct* FragmentType = ChangeList.ChangeTypes.GetTypeAtIndex(*It);

					const FConstStructView Fragment = ItemData::GetEntityFragment(Manager, Event.ItemHandle, FragmentType);
					if (Fragment.IsValid())
					{
						FragmentViews.Add(Fragment);
					}
				}

				ReplicationSubsystem.Server_UpdateFragments(Event.ItemHandle, ItemNetIDPtr->NetID, FragmentViews);
			}
			else if (Event.EventType == ItemData::Tags::FragmentRemove)
			{
				TArray<const UScriptStruct*> FragmentViews;
				FragmentViews.Reserve(ChangeList.ChangeTypes.CountStoredTypes());

				for (auto It = ChangeList.ChangeTypes.GetIndexIterator(); It; ++It)
				{
					const UScriptStruct* FragmentType = ChangeList.ChangeTypes.GetTypeAtIndex(*It);
					if (IsValid(FragmentType))
					{
						FragmentViews.Add(FragmentType);
					}
				}

				ReplicationSubsystem.Server_RemoveFragments(ItemNetIDPtr->NetID, FragmentViews);
			}
			else
			{
				// Unhandled EventType. Implement support here
				unimplemented()
			}
		}
		else if (ItemData::IsFaerieMassType(Event.ChangeType.Get()))
		{
			const UScriptStruct* Type = Event.ChangeType.Get();

			if (Event.EventType == ItemData::Tags::FragmentAdd ||
				Event.EventType == ItemData::Tags::FragmentGenericPropertyEdit)
			{
				const FConstStructView Fragment = ItemData::GetEntityFragment(Manager, Event.ItemHandle, Type);
				if (Fragment.IsValid())
				{
					ReplicationSubsystem.Server_UpdateFragments(Event.ItemHandle, ItemNetIDPtr->NetID, MakeConstArrayView(&Fragment, 1));
				}
			}
			else if (Event.EventType == ItemData::Tags::FragmentRemove)
			{
				ReplicationSubsystem.Server_RemoveFragments(ItemNetIDPtr->NetID, MakeConstArrayView(&Type, 1) );
			}
		}
		else if (Event.ChangeType == Container::FContainerEventPayload::StaticStruct())
		{
			// Sub-item was mutated, we don't care.
		}
		else
		{
			// Unhandled ChangeType. If a new payload was added, implement support here
			unimplemented()
		}
	}
}
