// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataEventCleanup.h"
#include "FaerieItemDataLog.h"
#include "FaerieItemEvent.h"
#include "DebuggingFlags.h"

#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataEventCleanup)

using namespace Faerie;

UFaerieItemDataEventCleanup::UFaerieItemDataEventCleanup()
  : EntityQuery(*this)
{
	// Cleanup events every frame no matter what created them.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);

	// We are the cleanup phase. Other processors should execute before us.
	ExecutionOrder.ExecuteInGroup = ItemData::EventCleanup;
}

void UFaerieItemDataEventCleanup::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
#if FAERIE_DEBUG
	// We do read the event data in debug mode.
	static constexpr EMassFragmentAccess EventAccess = EMassFragmentAccess::ReadOnly;
#else
	// We don't access the event data, we just require that it's there.
	static constexpr EMassFragmentAccess EventAccess = EMassFragmentAccess::None;
#endif
	EntityQuery.AddRequirement<ItemData::FMutationEvent>(EventAccess, EMassFragmentPresence::All);
}

void UFaerieItemDataEventCleanup::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// The only thing we do is destroy the events that were created this frame.
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
#if FAERIE_DEBUG
		const TConstArrayView<FMassEntityHandle> Entities = InContext.GetEntities();
		const TConstArrayView<ItemData::FMutationEvent> Events = InContext.GetFragmentView<ItemData::FMutationEvent>();
		check(Entities.Num() == Events.Num())
		for (int32 i = 0; i < Events.Num(); ++i)
		{
			FMassEntityHandle Entity = Entities[i];
			const ItemData::FMutationEvent& Event = Events[i];

			// @todo
		}
#endif
		InContext.Defer().DestroyEntities(InContext.GetEntities());
	});
}
