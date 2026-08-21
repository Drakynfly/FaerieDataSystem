// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieItemDataCreationObserver.h"
#include "MassReplication/FaerieMassReplicationSubsystem.h"

#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieMassFragment.h"

#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataCreationObserver)

UFaerieItemDataCreationObserver::UFaerieItemDataCreationObserver()
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

void UFaerieItemDataCreationObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FFaerieMassItemPointer>(EMassFragmentAccess::ReadOnly);
}

void UFaerieItemDataCreationObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	auto ReplicationSubsystem = UWorld::GetSubsystem<UFaerieMassReplicationSubsystem>(Context.GetWorld());

	EntityQuery.ForEachEntityChunk(Context, [ReplicationSubsystem](FMassExecutionContext& InContext)
	{
		const TConstArrayView<FFaerieMassItemPointer> ItemPointers = InContext.GetFragmentView<FFaerieMassItemPointer>();
		check(InContext.GetEntities().Num() == ItemPointers.Num());

		FMassEntityManager& Manager = InContext.GetEntityManagerChecked();

		for (int32 i = 0; i < ItemPointers.Num(); ++i)
		{
			// Build Item Instance from mass views.
			const FMassEntityHandle Entity = InContext.GetEntity(i);
			const FFaerieItemInstance Item(ItemPointers[i].Item.ResolveObjectPtr(), Entity);

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
				ReplicationSubsystem->Server_UpdateFragment(Item, FragmentViews);
			}
		}
	});
}