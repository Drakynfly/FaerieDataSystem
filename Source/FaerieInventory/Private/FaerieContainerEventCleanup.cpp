// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEventCleanup.h"
#include "FaerieContainerEvent.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemDataDefines.h"
#include "DebuggingFlags.h"

#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerEventCleanup)

using namespace Faerie;

UFaerieContainerEventCleanup::UFaerieContainerEventCleanup()
  : EntityQuery(*this)
{
	// Cleanup events every frame no matter what created them.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);

	// We are the cleanup phase. Other processors should execute before us.
	ExecutionOrder.ExecuteInGroup = Container::EventCleanup;
}

void UFaerieContainerEventCleanup::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
#if FAERIE_DEBUG
	// We do read the event data in debug mode.
	static constexpr EMassFragmentAccess EventAccess = EMassFragmentAccess::ReadOnly;
#else
	// We don't access the event data, we just require that it's there.
	static constexpr EMassFragmentAccess EventAccess = EMassFragmentAccess::None;
#endif
	EntityQuery.AddRequirement<Container::FContainerEventPayload>(EventAccess, EMassFragmentPresence::All);
}

void UFaerieContainerEventCleanup::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// The only thing we do is destroy the events that were created this frame.
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
#if FAERIE_DEBUG
		if (InContext.GetWorld()->GetNetMode() == ENetMode::NM_Client)
		{
			UE_LOGF(LogFaerieInventory, Log, "UFaerieContainerEventCleanup::Execute (client) %i events", InContext.GetNumEntities())
		}
		else
		{
			UE_LOGF(LogFaerieInventory, Log, "UFaerieContainerEventCleanup::Execute (server) %i events", InContext.GetNumEntities())
		}

		const TConstArrayView<FMassEntityHandle> Entities = InContext.GetEntities();
		const TConstArrayView<Container::FContainerEventPayload> Events = InContext.GetFragmentView<Container::FContainerEventPayload>();
		check(Entities.Num() == Events.Num())
		for (int32 i = 0; i < Events.Num(); ++i)
		{
			if (!Events[i].DebugCheck())
			{
				UE_LOGF(LogFaerieInventory, Warning, "EventCleanup: Event failed DebugCheck for entity: '%ls'", *LexToString(Entities[i]))
			}
		}
#endif
		InContext.Defer().DestroyEntities(InContext.GetEntities());
	});
}
