// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDestructionProcessor.h"
#include "FaerieItemEvent.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDestructionProcessor)

namespace Faerie::ItemData
{
	void MarkItemAsGarbage(FMassEntityManager& EntityManager, const FMassEntityHandle Item)
	{
		if (EntityManager.IsEntityValid(Item))
		{
			EntityManager.AddSparseElementToEntity<FMarkedAsGarbage>(Item);
		}
	}
}

using namespace Faerie;

UFaerieItemDestructionProcessor::UFaerieItemDestructionProcessor()
  : EntityQuery(*this)
{
	// Cleanup events every frame no matter what created them.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);

	// We are after the cleanup pass, so events still have valid item data for post-removal events.
	ExecutionOrder.ExecuteAfter.Add(ItemData::EventCleanup);
}
void UFaerieItemDestructionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddSparseRequirement<ItemData::FMarkedAsGarbage>(EMassFragmentPresence::All);
}

void UFaerieItemDestructionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Destroy the items that were marked since last pass.
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
		InContext.Defer().DestroyEntities(InContext.GetEntities());
	});
}
