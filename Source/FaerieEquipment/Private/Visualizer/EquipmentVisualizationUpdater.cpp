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
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& InContext)
	{
#if FAERIE_DEBUG
		if (InContext.GetWorld()->GetNetMode() == ENetMode::NM_Client)
		{
			UE_LOGF(LogFaerieEquipment, Verbose, "UFaerieVisualizationUpdater::Execute (client)")
		}
		else
		{
			UE_LOGF(LogFaerieEquipment, Verbose, "UFaerieVisualizationUpdater::Execute (server)")
		}
#endif

		const TConstArrayView<Container::FContainerEventPayload> Events = InContext.GetFragmentView<Container::FContainerEventPayload>();
		for (const Container::FContainerEventPayload& Event : Events)
		{
			UFaerieItemContainerBase* Container = Event.Container.Get();
			if (!IsValid(Container))
			{
				continue;
			}
			const FFaerieContainerExtensionVisualUpdater* VisualConfig = Container->ReadContainerData<FFaerieContainerExtensionVisualUpdater>(InContext.GetEntityManagerChecked(), true);
			if (!VisualConfig)
			{
				continue;
			}

			if (Event.IsRemovalEvent())
			{
				if (auto Slot = Cast<UFaerieItemStackContainer>(Container))
				{
					// If the whole stack is being removed, remove the visual for it
					if (!Slot->IsFilled())
					{
						auto&& Visualizer = GetVisualizer(Slot);
						if (!IsValid(Visualizer))
						{
							return;
						}
						Visualizer->RemoveVisualImpl(InContext.GetEntityManagerChecked(), FFaerieItemProxy(Slot));
					}
				}
			}
			else if (Event.IsAdditionEvent())
			{
				if (auto&& Stack = Cast<UFaerieItemStackContainer>(Container))
				{
					auto&& Visualizer = GetVisualizer(Stack);
					if (!IsValid(Visualizer))
					{
						return;
					}
					// A previously empty stack now has been filled with an item.
					Visualizer->CreateVisualImpl(InContext.GetEntityManagerChecked(), FFaerieItemProxy(Stack));
				}
			}
		}
	});
}


UEquipmentVisualizer* UFaerieVisualizationUpdater::GetVisualizer(const TNotNull<const UFaerieItemStackContainer*> Stack)
{
	AActor* OwningActor = Stack->GetTypedOuter<AActor>();
	if (!IsValid(OwningActor))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Stack (%ls) not owned by actor!", *Stack->GetName())
		return nullptr;
	}

	auto&& Visualizer = OwningActor->GetComponentByClass<UEquipmentVisualizer>();
	if (!IsValid(Visualizer))
	{
		UE_LOGF(LogFaerieEquipment, Warning, "GetVisualizer failed: Actor (%ls) does not have a visualizer component!", *OwningActor->GetName())
		return nullptr;
	}

	return Visualizer;
}