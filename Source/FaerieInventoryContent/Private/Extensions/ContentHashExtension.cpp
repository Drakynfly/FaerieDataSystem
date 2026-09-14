// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/ContentHashExtension.h"
#include "FaerieInventoryHashStatics.h"
#include "FaerieItemContainerBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "FaerieContainerEvent.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ContentHashExtension)

using namespace Faerie;

void FFaerieContainerContentHash::RecalcHash(const FMassEntityManager& EntityManager, UFaerieItemContainerBase* InContainer)
{
	// Everyone updates their local checksum.
	// @todo we need to check items recursively, not only top level.
	// @todo expose hashing algo to config
	LocalChecksum = Hash::HashContainer(InContainer, &EntityManager, &Hash::HashItemByName);

	// Clients do not update server checksum, they only receive it from the server.
	if (EntityManager.GetWorld()->GetNetMode() != ENetMode::NM_Client)
	{
		ServerChecksum = LocalChecksum;
	}
}

void UFaerieContainerContentHashView::SyncView()
{
	CheckAndBroadcast();
}

bool UFaerieContainerContentHashView::DoChecksumsMatch() const
{
	return ServerChecksum == LocalChecksum;
}

void UFaerieContainerContentHashView::CheckAndBroadcast()
{
	if (FStructView ContentHash = ContainerExtensionPtr.Value->Find(FFaerieContainerContentHash::StaticStruct(), false);
		ContentHash.IsValid())
	{
		const bool OldMatched = DoChecksumsMatch();
		UE_MVVM_SET_PROPERTY_VALUE(ServerChecksum, ContentHash.Get<FFaerieContainerContentHash>().ServerChecksum.Hash);
		UE_MVVM_SET_PROPERTY_VALUE(LocalChecksum, ContentHash.Get<FFaerieContainerContentHash>().LocalChecksum.Hash);
		const bool NewMatch = DoChecksumsMatch();
		if (OldMatched != NewMatch)
		{
			BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::DoChecksumsMatch);
		}
	}
}

UFaerieContainerContentHashUpdater::UFaerieContainerContentHashUpdater()
  : EventQuery(*this), ViewQuery(*this)
{
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieContainerContentHashUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ViewQuery.AddRequirement<Content::FContentHashViewFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerContentHashUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Track the containers we updated, so we can notify any active views of them.
	TSet<UObject*> ContainersUpdated;

	EventQuery.ForEachEntityChunk(Context, [&ContainersUpdated](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FEvent> Events = InContext.GetFragmentView<Container::FEvent>();
			for (const Container::FEvent& Event : Events)
			{
				UFaerieItemContainerBase* Container = Event.Container.Get();
				if (!IsValid(Container))
				{
					continue;
				}

				Container->WriteContainerData(FFaerieContainerContentHash::StaticStruct(), [Container, &InContext](const FStructView Element)
				{
					auto& ContentHash = Element.Get<FFaerieContainerContentHash>();
					ContentHash.RecalcHash(InContext.GetEntityManagerChecked(), Container);
				}, false);

				ContainersUpdated.Add(Container);
			}
		});

	ViewQuery.ForEachEntityChunk(Context, [&ContainersUpdated](const FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FViewModelFragment> Views = InContext.GetFragmentView<Container::FViewModelFragment>(Content::FContentHashViewFragment::StaticStruct());
		for (const Container::FViewModelFragment& ViewFragment : Views)
		{
			UFaerieContainerDataViewModelBase* View = Cast<UFaerieContainerDataViewModelBase>(ViewFragment.ViewObject.Get());
			if (!IsValid(View))
			{
				continue;
			}

			if (ContainersUpdated.Contains(View->GetContainerObject()))
			{
				View->SyncView();
			}
		}
	});
}