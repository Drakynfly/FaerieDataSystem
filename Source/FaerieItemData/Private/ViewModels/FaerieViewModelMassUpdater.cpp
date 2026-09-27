// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ViewModels/FaerieViewModelMassUpdater.h"

#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include "MassExecutionContext.h"

#include "ViewModels/FaerieItemDataViewFieldChangeInterface.h"
#include "ViewModels/FaerieMassEntityViewModelBase.h"
#include "ViewModels/FaerieViewModelSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieViewModelMassUpdater)

using namespace Faerie;

UFaerieViewModelFieldUpdater::UFaerieViewModelFieldUpdater()
  : EventQuery(*this), ViewQuery(*this)
{
	// Process everywhere.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);

	ExecutionOrder.ExecuteBefore.Add(ItemData::EventCleanup);

	bRequiresGameThreadExecution = true;

	bAllowMultipleInstances = true;
	bAutoRegisterWithProcessingPhases = false;
}

void UFaerieViewModelFieldUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// Event query tells us which entities to update view models for.
	EventQuery.AddRequirement<ItemData::FMutationEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	EventQuery.AddRequirement<ItemData::FFieldChangePayload>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);

	// View query finds all View Model entities for this view model type.
	ViewQuery.AddRequirement(ViewModelFragmentType, EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieViewModelFieldUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Collect item handles and their field change masks.
	TCompactMap<FMassEntityHandle, ItemData::FFieldChangePayload> EntitiesChanged;

	EventQuery.ForEachEntityChunk(Context, [this, &EntitiesChanged](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<ItemData::FMutationEvent> Mutations = InContext.GetFragmentView<ItemData::FMutationEvent>();
			const TConstArrayView<ItemData::FFieldChangePayload> FieldChange = InContext.GetFragmentView<ItemData::FFieldChangePayload>();
			check(Mutations.Num() == FieldChange.Num());

			for (int32 i = 0; i < Mutations.Num(); ++i)
			{
				if (Mutations[i].ChangeType == ItemDataFragmentType)
				{
					EntitiesChanged.FindOrAdd(Mutations[i].ItemHandle) |= FieldChange[i];
				}
			}
		});

	ViewQuery.ForEachEntityChunk(Context, [this, &EntitiesChanged](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FViewModelFragment> Views = InContext.GetFragmentView<Container::FViewModelFragment>(ViewModelFragmentType);
			for (const Container::FViewModelFragment& ViewFragment : Views)
			{
				IFaerieItemDataViewFieldChangeInterface* View = Cast<IFaerieItemDataViewFieldChangeInterface>(ViewFragment.ViewObject.Get());
				if (!View)
				{
					continue;
				}

				if (const ItemData::FFieldChangePayload* Payload = EntitiesChanged.Find(View->GetItemHandle()))
				{
					View->OnFieldChange(InContext.GetEntityManagerChecked(), *Payload);
				}
			}
		});
}

UFaerieViewModelEntityDestructionNotifier::UFaerieViewModelEntityDestructionNotifier()
  : EntityQuery(*this)
{
	// Observe destruction of entities with an item pointer.
	ObservedTypes = { FFaerieMassItemPointer::StaticStruct() };
	ObservedOperations = EMassObservedOperationFlags::DestroyEntity;

	// Process everywhere.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::All);

	bRequiresGameThreadExecution = true;
}

void UFaerieViewModelEntityDestructionNotifier::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	ProcessorRequirements.AddSubsystemRequirement<UFaerieViewModelSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UFaerieViewModelEntityDestructionNotifier::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UFaerieViewModelSubsystem& ViewModelSubsystem = Context.GetMutableSubsystemChecked<UFaerieViewModelSubsystem>();

	// @Todo ...
}