// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieItemDataMassReplicators.h"
#include "MassReplication/FaerieMassReplicationSubsystem.h"

#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieMassFragment.h"

#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataMassReplicators)

using namespace Faerie;

UFaerieItemDataCreationReplicator::UFaerieItemDataCreationReplicator()
  : EntityQuery(*this)
{
	// Observe creation of entities with an item pointer.
	ObservedTypes = { FFaerieMassItemPointer::StaticStruct() };
	ObservedOperations = EMassObservedOperationFlags::CreateEntity;

	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);

	// We broadcast events to the replication subsystem, so we need to run on the game thread.
	bRequiresGameThreadExecution = true;
}

void UFaerieItemDataCreationReplicator::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FFaerieMassItemPointer>(EMassFragmentAccess::None); // Require presence, but we don't read or write.
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataCreationReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
	{
		const TConstArrayView<FMassEntityHandle> Entities = InContext.GetEntities();

		FMassEntityManager& Manager = InContext.GetEntityManagerChecked();

		for (int32 i = 0; i < Entities.Num(); ++i)
		{
			// Build Item Instance from mass views.
			const FMassEntityHandle Entity = InContext.GetEntity(i);

			// Create view for each dirty fragment.
			TArray<TConstStructView<FFaerieMassFragment>> FragmentViews;

			const FMassArchetypeHandle Archetype = Manager.GetArchetypeForEntity(Entity);
			Manager.ForEachArchetypeFragmentType(Archetype,
				[Entity, &Manager, &FragmentViews](const UScriptStruct* FragmentType)
				{
					if (!FragmentType->IsChildOf<FFaerieMassFragment>())
					{
						return;
					}

					FragmentViews.Add(Manager.GetFragmentDataStruct(Entity, FragmentType).Get<FFaerieMassFragment>());
				});

			if (!FragmentViews.IsEmpty())
			{
				// Push all new fragments into the replication subsystem.
				ReplicationSubsystem.Server_UpdateFragments(Manager, Entity, FragmentViews);
			}
		}
	});
}

UFaerieItemDataDeletionReplicator::UFaerieItemDataDeletionReplicator()
  : EntityQuery(*this)
{
	// Observe destruction of entities with an item pointer.
	ObservedTypes = { FFaerieMassItemPointer::StaticStruct() };
	ObservedOperations = EMassObservedOperationFlags::DestroyEntity;

	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);

	// We broadcast events to the replication subsystem, so we need to run on the game thread.
	bRequiresGameThreadExecution = true;
}

void UFaerieItemDataDeletionReplicator::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataDeletionReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EntityQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
		{
			ReplicationSubsystem.Server_RemoveEntities(InContext.GetEntityManagerChecked(), InContext.GetEntities());
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
	ProcessorRequirements.AddSubsystemRequirement<UFaerieMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemDataChangeReplicator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieMassReplicationSubsystem& ReplicationSubsystem = Context.GetMutableSubsystemChecked<UFaerieMassReplicationSubsystem>();

	EventQuery.ForEachEntityChunk(Context, [&ReplicationSubsystem](FMassExecutionContext& InContext)
	{
		FMassEntityManager& Manager = InContext.GetEntityManagerChecked();

		const TConstArrayView<ItemData::FMutationEvent> Events = InContext.GetFragmentView<ItemData::FMutationEvent>();
		if (Events.IsEmpty()) return;

		for (const ItemData::FMutationEvent& Event : Events)
		{
			if (Event.EventType == ItemData::Tags::FragmentAdd)
			{
				if (Event.ChangeType == ItemData::FMutationPayloadChangeList::StaticStruct())
				{
					// @todo
					unimplemented()
				}
				else
				{
					const TConstStructView<FFaerieMassFragment> Fragment = Manager.GetFragmentDataStruct(Event.ItemHandle,
						Event.ChangeType.Get()).Get<FFaerieMassFragment>();
					ReplicationSubsystem.Server_UpdateFragments(InContext.GetEntityManagerChecked(), Event.ItemHandle,
						MakeConstArrayView(&Fragment, 1));
				}
			}
			else if (Event.EventType == ItemData::Tags::FragmentRemove)
			{
				if (Event.ChangeType == ItemData::FMutationPayloadChangeList::StaticStruct())
				{
					// @todo
					unimplemented()
				}
				else
				{
					const UScriptStruct* Type = Event.ChangeType.Get();
					ReplicationSubsystem.Server_RemoveFragments(InContext.GetEntityManagerChecked(), Event.ItemHandle,
						MakeConstArrayView(&Type, 1) );
				}
			}
		}
	});
}
