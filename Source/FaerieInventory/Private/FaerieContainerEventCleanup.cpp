// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEventCleanup.h"
#include "FaerieContainerEvent.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerEventCleanup)

UFaerieContainerEventCleanup::UFaerieContainerEventCleanup()
	: EntityQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteInGroup = Faerie::Container::EventCleanup;
}

void UFaerieContainerEventCleanup::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// We don't access the event data, we just require that it's there.
	EntityQuery.AddRequirement<Faerie::Container::FEvent>(EMassFragmentAccess::None, EMassFragmentPresence::All);
}

void UFaerieContainerEventCleanup::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// The only thing we do is destroy the events that were created this frame.
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& InContext)
	{
		InContext.Defer().DestroyEntities(InContext.GetEntities());
	});
}
