// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassProcessor.h"

#include "UObject/Interface.h"
#include "UObject/WeakInterfacePtr.h"

#include "ContainerEventSubscription.generated.h"

#define FAE_API FAERIEINVENTORYCONTENT_API

namespace Faerie::Container
{
	struct FEvent;
}

class UFaerieItemContainerBase;

UINTERFACE(NotBlueprintable)
class UFaerieContainerEventSubscriber : public UInterface
{
	GENERATED_BODY()
};

class IFaerieContainerEventSubscriber
{
	GENERATED_BODY()

	// This class calls our functions. Nothing else should.
	friend class UFaerieContainerEventSubscriptionUpdater;

protected:
	virtual void OnContainerEventBatch(TNotNull<UFaerieItemContainerBase*> Container, TConstArrayView<const Faerie::Container::FEvent*> Events) = 0;
};

namespace Faerie::Content
{
	USTRUCT()
	struct FContainerEventSubscribers
	{
		GENERATED_BODY()

		TArray<TWeakInterfacePtr<IFaerieContainerEventSubscriber>> Subscribers;
	};

	FAE_API void SubscribeToContainerEvents(TNotNull<UFaerieItemContainerBase*> Container, TNotNull<IFaerieContainerEventSubscriber*> Object);
	FAE_API void UnsubscribeFromContainerEvents(TNotNull<UFaerieItemContainerBase*> Container, TNotNull<IFaerieContainerEventSubscriber*> Object);
}

/*
 * Notifies arbitrary objects about events in a container. The objects must implement IFaerieContainerEventSubscriber.
 * Use SubscribeToContainerEvents / UnsubscribeFromContainerEvents add/remove implementations.
 */
UCLASS()
class UFaerieContainerEventSubscriptionUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerEventSubscriptionUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
};

#undef FAE_API