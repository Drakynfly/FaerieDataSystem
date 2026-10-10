// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemOwnerNotifier.h"
#include "FaerieItem.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemEvent.h"
#include "MassExecutionContext.h"

#include "Fragments/FaerieItemOwningContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemOwnerNotifier)

using namespace Faerie;

UFaerieItemOwnerNotifier::UFaerieItemOwnerNotifier()
  : EventQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteBefore.Add(ItemData::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieItemOwnerNotifier::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<ItemData::FMutationEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieItemOwnerNotifier::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EventQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
		const TConstArrayView<ItemData::FMutationEvent> Events = InContext.GetFragmentView<ItemData::FMutationEvent>();
		if (Events.IsEmpty()) return;

		for (const ItemData::FMutationEvent& Event : Events)
		{
			if (!InContext.GetEntityManagerChecked().IsEntityValid(Event.ItemHandle))
			{
				// Item no longer exists, skip.
				continue;
			}

			if (const FFaerieMassItemOwningContainer* OwnerFragment = InContext.GetEntityManagerChecked().GetConstSharedFragmentDataPtr<FFaerieMassItemOwningContainer>(Event.ItemHandle))
			{
				if (IsValid(OwnerFragment->Owner))
				{
					OwnerFragment->Owner->OnItemDataChanged(Event.ItemHandle, Event.EventType);
				}
			}
		}
	});
}
