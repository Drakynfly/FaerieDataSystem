// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Visualizer/EquipmentVisualizationUpdater.h"
#include "Visualizer/EquipmentVisualizer.h"

#include "FaerieEquipmentLog.h"
#include "FaerieItemContainerBase.h"
#include "MassExecutionContext.h"
#include "FaerieContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizationUpdater)

using namespace Faerie;

UFaerieVisualizationUpdater::UFaerieVisualizationUpdater()
  : EntityQuery(*this)
{
#if UE_SERVER
	// Servers do not need to create visuals.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::None);
#else
	// Update everywhere on non-server builds. Listen servers and clients both need visuals.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);
#endif

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We send events to game-thread systems.
	// @Todo if updating visuals were made thread safe, we could remove this.
	bRequiresGameThreadExecution = true;
}

void UFaerieVisualizationUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<Container::FContainerEventPayload>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieVisualizationUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, ProcessEvents);
}

UEquipmentVisualizer* UFaerieVisualizationUpdater::GetVisualizer(const TNotNull<const UObject*> ContextObject)
{
	const AActor* OwningActor = ContextObject->GetTypedOuter<AActor>();
	if (!IsValid(OwningActor))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: ContextObject (%ls) not owned by actor!", *ContextObject->GetName())
		return nullptr;
	}

	UEquipmentVisualizer* Visualizer = OwningActor->GetComponentByClass<UEquipmentVisualizer>();
	if (!IsValid(Visualizer))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Actor (%ls) does not have a visualizer component!", *OwningActor->GetName())
		return nullptr;
	}

	return Visualizer;
}

void UFaerieVisualizationUpdater::ProcessEvents(FMassExecutionContext& Context)
{
#if FAERIE_DEBUG
	if (Context.GetWorld()->GetNetMode() == ENetMode::NM_Client)
	{
		UE_LOGF(LogFaerieEquipment, Verbose, "VisualizationUpdater::ProcessEvents (client)")
	}
	else
	{
		UE_LOGF(LogFaerieEquipment, Verbose, "VisualizationUpdater::ProcessEvents (server)")
	}
#endif

	const TConstArrayView<Container::FContainerEventPayload> Events = Context.GetFragmentView<Container::FContainerEventPayload>();
	for (const Container::FContainerEventPayload& Event : Events)
	{
		const UFaerieItemContainerBase* Container = Event.Container.Get();
		if (!IsValid(Container))
		{
			UE_LOGF(LogFaerieEquipment, Warning, "VisualizationUpdater: Skipping event with invalid Container!")
			continue;
		}

		UEquipmentVisualizer* Visualizer = GetVisualizer(Container);
		if (!IsValid(Visualizer))
		{
			continue;
		}

		Visualizer->FlushInvalid(Context.GetEntityManagerChecked());

		if (Event.IsCopiesChangeEvent())
		{
			// Ignore copies change events, they don't affect visuals.
			// @todo but what if they do...? add separate code path to update an existing visual with a new stack count, e.g, a quiver that displays its stack count as arrows.
			continue;
		}

		if (Event.IsRemovalEvent())
		{
			// Flush invalid should take care of removals. The Proxy call below would fail anyway for removed items.
			continue;
		}

		const FFaerieItemProxy Proxy = Container->Proxy(Event.Instance.GetMassEntityHandle());
		if (!Proxy.IsValid())
		{
			UE_LOGF(LogFaerieEquipment, Warning, "VisualizationUpdater: Failed to create proxy for instance!")
			continue;
		}

		Visualizer->RefreshVisual(Context.GetEntityManagerChecked(), Proxy);
	}
}
