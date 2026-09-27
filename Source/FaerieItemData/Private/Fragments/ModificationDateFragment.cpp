// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItem.h"
#include "FaerieItemEvent.h"
#include "MassExecutionContext.h"

#include "Fragments/ModificationDateFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModificationDateFragment)

using namespace Faerie;

UFaerieItemModificationDataInitializer::UFaerieItemModificationDataInitializer()
 : EntityQuery(*this)
{
	// Observe creation of entities with a modification data.
	ObservedTypes = { FFaerieItemModificationDate::StaticStruct() };
	ObservedOperations = EMassObservedOperationFlags::CreateEntity;

	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server);
}

void UFaerieItemModificationDataInitializer::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FFaerieItemModificationDate>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemModificationDataInitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
		{
			const TArrayView<FFaerieItemModificationDate> Events = InContext.GetMutableFragmentView<FFaerieItemModificationDate>();
			if (Events.IsEmpty()) return;

			for (FFaerieItemModificationDate& DateFragment : Events)
			{
				// Assign new value if initialized to none
				if (DateFragment.LastModified == FDateTime())
				{
					DateFragment.LastModified = FDateTime::UtcNow();
				}
			}
		});
}

UFaerieItemModificationDateUpdater::UFaerieItemModificationDateUpdater()
  : EventQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteBefore.Add(ItemData::EventCleanup);
}

void UFaerieItemModificationDateUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<ItemData::FMutationEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	EventQuery.AddIndirectFragmentRequirement<FFaerieItemModificationDate>(EMassFragmentAccess::ReadWrite);
}

void UFaerieItemModificationDateUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EventQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
		const TConstArrayView<ItemData::FMutationEvent> Events = InContext.GetFragmentView<ItemData::FMutationEvent>();
		if (Events.IsEmpty()) return;

		for (const ItemData::FMutationEvent& Event : Events)
		{
			if (FFaerieItemModificationDate* DateFragment = InContext.GetIndirectFragmentPtr<FFaerieItemModificationDate>(Event.ItemHandle))
			{
				// Assign new value
				DateFragment->LastModified = FDateTime::UtcNow();
			}
		}
	});
}
