// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Visualizer/EquipmentVisualizationUpdater.h"
#include "Visualizer/EquipmentVisualizer.h"

#include "FaerieEquipmentLog.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStackContainer.h"
#include "MassExecutionContext.h"
#include "FaerieContainerEvent.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

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

UEquipmentVisualizer* UFaerieVisualizationUpdater::GetVisualizer(const FFaerieItemProxy& Proxy)
{
	const UObject* OwningObject = Proxy.GetProxyObject();
	if (!IsValid(OwningObject))
	{
		// If there isn't a UObject stored in the proxy, fallback to the Item's owner.
		OwningObject = Proxy.GetItemOwner();

		if (!IsValid(OwningObject))
		{
			UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: No UObject associated with Proxy!")
			return nullptr;
		}
	}

	const AActor* OwningActor = OwningObject->GetTypedOuter<AActor>();
	if (!IsValid(OwningActor))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Proxy not owned by actor!")
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
		UE_LOGF(LogFaerieEquipment, Verbose, "UFaerieVisualizationUpdater::ProcessEvents (client)")
	}
	else
	{
		UE_LOGF(LogFaerieEquipment, Verbose, "UFaerieVisualizationUpdater::ProcessEvents (server)")
	}
#endif

	const TConstArrayView<Container::FContainerEventPayload> Events = Context.GetFragmentView<Container::FContainerEventPayload>();
	for (const Container::FContainerEventPayload& Event : Events)
	{
		UFaerieItemContainerBase* Container = Event.Container.Get();
		if (!IsValid(Container))
		{
			continue;
		}

		UFaerieItemStackContainer* Stack = Cast<UFaerieItemStackContainer>(Container);
		if (!IsValid(Stack))
		{
			continue;
		}

		const FFaerieItemProxy StackProxy(Stack);
		UEquipmentVisualizer* Visualizer = GetVisualizer(StackProxy);
		if (IsValid(Visualizer))
		{
			Visualizer->RefreshVisual(Context.GetEntityManagerChecked(), StackProxy);
		}
	}
}
