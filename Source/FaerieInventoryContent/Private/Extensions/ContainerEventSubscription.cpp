// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"

#include "Extensions/ContainerEventSubscription.h"

#include "FaerieContainerEvent.h"
#include "FaerieItemContainerBase.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContainerEventSubscription)

namespace Faerie::Content
{
	void SubscribeToContainerEvents(const TNotNull<UFaerieItemContainerBase*> Container, const TNotNull<IFaerieContainerEventSubscriber*> Object)
	{
		// @Todo permissions check in-case we want to stop this from being created??
		static constexpr bool CreateIfMissing = true;

		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Container->GetWorld());
		Container->WriteContainerData(EntityManager, FContainerEventSubscribers::StaticStruct(),
			[Object](const FStructView Element)
			{
				Element.Get<FContainerEventSubscribers>().Subscribers.Add(NotNullGet(Object));
			}, CreateIfMissing);
	}

	void UnsubscribeFromContainerEvents(const TNotNull<UFaerieItemContainerBase*> Container, const TNotNull<IFaerieContainerEventSubscriber*> Object)
	{
		// Don't create data, just to remove from it.
		static constexpr bool CreateIfMissing = false;

		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(Container->GetWorld());
		Container->WriteContainerData(EntityManager, FContainerEventSubscribers::StaticStruct(),
			[Object](const FStructView Element)
			{
				Element.Get<FContainerEventSubscribers>().Subscribers.Remove(NotNullGet(Object));
			}, CreateIfMissing);
	}
}

using namespace Faerie;

UFaerieContainerEventSubscriptionUpdater::UFaerieContainerEventSubscriptionUpdater()
  : EventQuery(*this)
{
	// Process only on the client.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieContainerEventSubscriptionUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<Container::FContainerEventPayload>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerEventSubscriptionUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EventQuery.ForEachEntityChunk(Context, [](const FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FContainerEventPayload> Events = InContext.GetFragmentView<Container::FContainerEventPayload>();
		if (Events.IsEmpty()) return;

		// Collated events per container for this chunk. Uses IndirectArray to avoid copying the events.
		TCompactMap<UFaerieItemContainerBase*, TArray<const Container::FContainerEventPayload*>> EventsPerContainer;

		for (const Container::FContainerEventPayload& Event : Events)
		{
			UFaerieItemContainerBase* Container = Event.Container.Get();
			if (!IsValid(Container))
			{
				continue;
			}

			EventsPerContainer.FindOrAdd(Container).Add(&Event);
		}

		for (auto&& ContainerEvents : EventsPerContainer)
		{
			UFaerieItemContainerBase* Container = ContainerEvents.Key;
			if (const Content::FContainerEventSubscribers* SubscriberList = Container->ReadContainerData<Content::FContainerEventSubscribers>(
				InContext.GetEntityManagerChecked(), true))
			{
				for (const TWeakInterfacePtr<IFaerieContainerEventSubscriber>& WeakSubscriber : SubscriberList->Subscribers)
				{
					if (IFaerieContainerEventSubscriber* Subscriber = WeakSubscriber.Get())
					{
						Subscriber->OnContainerEventBatch(Container, ContainerEvents.Value);
					}
				}
			}

			// @todo purge invalid subscribers???
		}
	});
}