// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include "Extensions/InventoryLoggerExtension.h"
#include "FaerieItemContainerBase.h"
#include "MassExecutionContext.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryLoggerExtension)

using namespace Faerie;

void UFaerieContainerEventLogView::SyncView()
{
	RecalculateEventTags();
	RecalculateLogView();
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::GetNumEvents);
}

bool UFaerieContainerEventLogView::AreEventsLogged() const
{
	if (ContainerExtensionPtr.Key.IsValid())
	{
		return ContainerExtensionPtr.Value->Find(FFaerieContainerEventLog::StaticStruct(), false).IsValid();
	}
	return false;
}

int32 UFaerieContainerEventLogView::GetNumEvents() const
{
	if (ContainerExtensionPtr.Key.IsValid())
	{
		if (const FStructView View = ContainerExtensionPtr.Value->Find(FFaerieContainerEventLog::StaticStruct(), false);
			View.IsValid())
		{
			return View.Get<FFaerieContainerEventLog>().EventLog.Num();
		}
	}
	return 0;
}

FFaerieBlueprintInventoryEvent UFaerieContainerEventLogView::GetEvent(int32 Index, const bool FromEnd) const
{
	if (ContainerExtensionPtr.Key.IsValid())
	{
		const FStructView View = ContainerExtensionPtr.Value->Find(FFaerieContainerEventLog::StaticStruct(), false);
		if (!View.IsValid()) return FFaerieBlueprintInventoryEvent();

		auto&& Log = View.Get<FFaerieContainerEventLog>().EventLog;

		if (FromEnd)
		{
			Index = Log.Num() - 1 - Index;
		}

		if (Log.IsValidIndex(Index))
		{
			return Log[Index];
		}
	}
	return FFaerieBlueprintInventoryEvent();
}

void UFaerieContainerEventLogView::PreviousPageIndex()
{
	SetPageIndex(FMath::Max(PageIndex - 1, 0));
}

void UFaerieContainerEventLogView::NextPageIndex()
{
	SetPageIndex(FMath::Min(PageIndex + 1, NumPages - 1));
}

void UFaerieContainerEventLogView::SetPageIndex(const int32 Index)
{
	if (Index != PageIndex)
	{
		PageIndex = Index;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::PageIndex);
		RecalculateLogView();
	}
}

void UFaerieContainerEventLogView::SetCountPerPage(const int32 Count)
{
	if (Count != CountPerPage)
	{
		CountPerPage = Count;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::CountPerPage);
		RecalculateLogView();
	}
}

void UFaerieContainerEventLogView::SetFilterTags(const FGameplayTagContainer Tags)
{
	if (Tags != FilterTags)
	{
		FilterTags = Tags;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::FilterTags);
		RecalculateLogView();
	}
}

void UFaerieContainerEventLogView::SetInvertEventOrder(const bool Invert)
{
	if (Invert != InvertEventOrder)
	{
		InvertEventOrder = Invert;
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::InvertEventOrder);
		RecalculateLogView();
	}
}

void UFaerieContainerEventLogView::RecalculateEventTags()
{
	if (!ContainerExtensionPtr.Key.IsValid())
	{
		return;
	}

	FGameplayTagContainer NewEventTags;

	const FStructView View = ContainerExtensionPtr.Value->Find(FFaerieContainerEventLog::StaticStruct(), false);
	if (!View.IsValid())
	{
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::AreEventsLogged);
		return;
	}

	auto&& Log = View.Get<FFaerieContainerEventLog>().EventLog;

	for (auto&& Event : Log)
	{
		NewEventTags.AddTag(Event.Type);
	}

	UE_MVVM_SET_PROPERTY_VALUE(EventTags, NewEventTags);
}

void UFaerieContainerEventLogView::RecalculateLogView()
{
	if (!ContainerExtensionPtr.Key.IsValid())
	{
		return;
	}

	PageView.Empty(CountPerPage);
	NumFilteredEvents = 0;

	const FStructView View = ContainerExtensionPtr.Value->Find(FFaerieContainerEventLog::StaticStruct(), false);
	if (!View.IsValid()) return;

	auto&& Log = View.Get<FFaerieContainerEventLog>().EventLog;

	const int32 SkipCount = PageIndex * CountPerPage;
	if (InvertEventOrder)
	{
		if (FilterTags.IsValid())
		{
			for (int32 i = Log.Num() - 1 - SkipCount; i >= 0; --i)
			{
				if (Log[i].Type.MatchesAny(FilterTags))
				{
					continue;
				}

				NumFilteredEvents++;
				if (PageView.Num() < CountPerPage)
				{
					PageView.Add(Log[i]);
				}
			}
		}
		else
		{
			NumFilteredEvents = Log.Num();
			PageView = MakeConstArrayView(Log).Mid(Log.Num() - 1 - SkipCount - CountPerPage, CountPerPage);
			Algo::Reverse(PageView);
		}
	}
	else
	{
		if (FilterTags.IsValid())
		{
			for (int32 i = SkipCount; i < Log.Num(); ++i)
			{
				if (Log[i].Type.MatchesAny(FilterTags))
				{
					continue;
				}

				NumFilteredEvents++;
				if (PageView.Num() < CountPerPage)
				{
					PageView.Add(Log[i]);
				}
			}
		}
		else
		{
			NumFilteredEvents = Log.Num();
			PageView = MakeConstArrayView(Log).Mid(SkipCount - 1, CountPerPage);
		}
	}

	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::NumFilteredEvents);
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::PageView);

	NumPages = NumFilteredEvents / CountPerPage + 1;
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::NumPages);

	// Clamp page index to valid value.
	SetPageIndex(FMath::Min(PageIndex, NumPages-1));
}

UFaerieContainerEventLogUpdater::UFaerieContainerEventLogUpdater()
  : EventQuery(*this), ViewQuery(*this)
{
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieContainerEventLogUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ViewQuery.AddRequirement<Content::FEventLogViewFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieContainerEventLogUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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

			Container->WriteContainerData(FFaerieContainerEventLog::StaticStruct(), [&Event](const FStructView Element)
			{
				auto& Log = Element.Get<FFaerieContainerEventLog>();

				// Remove oldest event, if we are at limit.
				// @Todo can we store events in a container that isn't terrible at popping the first element.
				if (Log.EventLog.Num() == Log.MaxEventsToStore)
				{
					Log.EventLog.RemoveAt(0);
				}

				Log.EventLog.Emplace(FFaerieBlueprintInventoryEvent::FromNativeEvent(Event));
			}, false);

			ContainersUpdated.Add(Container);
		}
	});

	ViewQuery.ForEachEntityChunk(Context, [&ContainersUpdated](const FMassExecutionContext& InContext)
	{
		const TConstArrayView<Container::FViewModelFragment> Views = InContext.GetFragmentView<Container::FViewModelFragment>(Content::FEventLogViewFragment::StaticStruct());
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