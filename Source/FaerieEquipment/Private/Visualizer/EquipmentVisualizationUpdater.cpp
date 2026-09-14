// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include "Visualizer/EquipmentVisualizationUpdater.h"
#include "Visualizer/EquipmentVisualizer.h"

#include "FaerieEquipmentLog.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStackContainer.h"
#include "MassExecutionContext.h"

#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentVisualizationUpdater)

using namespace Faerie;

UFaerieVisualizationUpdater::UFaerieVisualizationUpdater()
  : EntityQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We send events to game-thread systems.
	// @Todo if updating visuals were made thread safe, we could remove this.
	bRequiresGameThreadExecution = true;
}

void UFaerieVisualizationUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieVisualizationUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FEvent> Events = InContext.GetFragmentView<Container::FEvent>();
		for (const Container::FEvent& Event : Events)
		{
			UFaerieItemContainerBase* Container = Event.Container.Get();
			if (!IsValid(Container))
			{
				continue;
			}
			const FFaerieContainerExtensionVisualUpdater* VisualConfig = Container->ReadContainerData<FFaerieContainerExtensionVisualUpdater>(true);
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
						Visualizer->RemoveVisualImpl(FFaerieItemProxy(Slot));
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
					Visualizer->CreateVisualImpl(FFaerieItemProxy(Stack));
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