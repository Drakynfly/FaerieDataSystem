// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/ContainerEventSubscription.h"

#include "FaerieContainerEvent.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContainerEventSubscription)

namespace Faerie::Content
{
	void SubscribeToContainerEvents(const TNotNull<UFaerieItemContainerBase*> Container, const TNotNull<IFaerieContainerEventSubscriber*> Object)
	{
		// @Todo permissions check in-case we want to stop this from being created??
		static constexpr bool CreateIfMissing = true;

		Container->WriteContainerData(FContainerEventSubscribers::StaticStruct(),
			[Object](const FStructView Element)
			{
				Element.Get<FContainerEventSubscribers>().Subscribers.Add(NotNullGet(Object));
			}, CreateIfMissing);
	}

	void UnsubscribeFromContainerEvents(const TNotNull<UFaerieItemContainerBase*> Container, const TNotNull<IFaerieContainerEventSubscriber*> Object)
	{
		// Don't create data, just to remove from it.
		static constexpr bool CreateIfMissing = false;

		Container->WriteContainerData(FContainerEventSubscribers::StaticStruct(),
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
	EventQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerEventSubscriptionUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EventQuery.ForEachEntityChunk(Context, [this](const FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FEvent> Events = InContext.GetFragmentView<Container::FEvent>();
		if (Events.IsEmpty()) return;

		// Collated events per container for this chunk. Uses IndirectArray to avoid copying the events.
		TCompactMap<UFaerieItemContainerBase*, TArray<const Container::FEvent*>> EventsPerContainer;

		for (const Container::FEvent& Event : Events)
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
			if (const Content::FContainerEventSubscribers* SubscriberList = Container->ReadContainerData<Content::FContainerEventSubscribers>(true))
			{
				for (const TWeakInterfacePtr<IFaerieContainerEventSubscriber>& WeakSubscriber : SubscriberList->Subscribers)
				{
					if (IFaerieContainerEventSubscriber* Subscriber = WeakSubscriber.Get())
					{
						Subscriber->OnContainerEventBatch(Container,
							MakeConstArrayView(ContainerEvents.Value.GetData(), ContainerEvents.Value.Num()));
					}
				}
			}

			// @todo purge invalid subscribers???
		}
	});
}