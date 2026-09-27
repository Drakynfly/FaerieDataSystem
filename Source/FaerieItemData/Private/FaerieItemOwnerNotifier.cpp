// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemOwnerNotifier.h"
#include "FaerieItem.h"
#include "FaerieItemEvent.h"
#include "FaerieItemOwnerInterface.h"
#include "MassExecutionContext.h"

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
			if (const FFaerieMassItemOwner* OwnerFragment = InContext.GetEntityManagerChecked().GetConstSharedFragmentDataPtr<FFaerieMassItemOwner>(Event.ItemHandle))
			{
				if (IFaerieItemOwnerInterface* Interface = OwnerFragment->GetInterface())
				{
					FFaerieItemInstance Instance(nullptr, Event.ItemHandle);
					Interface->OnItemDataChanged(Instance, Event.EventType);
				}
			}
		}
	});
}
