// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"

#include "Extensions/ItemContainerCountLimit.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryContentLog.h"
#include "MassExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerCountLimit)

using namespace Faerie;

void FFaerieItemContainerCountLimit::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	EntryAmountCache.Reset();
	CurrentTotalItemCopies = 0;

	for (auto It = Container::KeyRange(Container); It; ++It)
	{
		UpdateCacheForEntry(It.GetKey(), It.GetCopies());
	}
}

EFaerieExtensionResponse FFaerieItemContainerCountLimit::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
	const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, const FFaerieExtensionAllowsAdditionArgs Args) const
{
	int32 TestCount = 0;

	// Sum all stacks
	for (int32 i = 0; i < Proxies.Num(); ++i)
	{
		const FFaerieItemProxy Proxy = Proxies[i];
		TestCount += Proxy.GetCopies();
	}

	if (!CanContain(TestCount))
	{
		UE_LOGF(LogFaerieInventoryContent, VeryVerbose,
			"AllowsAddition: Cannot add Stack(s) (Total Count: %i)",
			TestCount);

		return EFaerieExtensionResponse::Disallowed;
	}

	return EFaerieExtensionResponse::NoExplicitResponse;
}

void FFaerieItemContainerCountLimit::PostEvent(const Container::FEvent& Event)
{
	if (Event.EntryRemoved)
	{
		// Entry was removed, delete cache.
		RemoveCacheForEntry(Event.EntryTouched);
	}
	else
	{
		if (Event.IsRemovalEvent())
		{
			// Event is removal, pass negative Copies as delta
			UpdateCacheForEntry(Event.EntryTouched, -Event.Copies);
		}
		else
		{
			UpdateCacheForEntry(Event.EntryTouched, Event.Copies);
		}
	}
}

void FFaerieItemContainerCountLimit::SetMaxInstanceCount(const int32 Count)
{
	MaxInstanceCount = Count;
}

void FFaerieItemContainerCountLimit::SetMaxEntryCount(const int32 Count)
{
	MaxEntryCount = Count;
}

int32 FFaerieItemContainerCountLimit::GetTotalItemCount() const
{
	return CurrentTotalItemCopies;
}

int32 FFaerieItemContainerCountLimit::GetRemainingEntryCount() const
{
	if (MaxEntryCount <= 0)
	{
		return ItemData::UnlimitedStack;
	}
	return MaxEntryCount - EntryAmountCache.Num();
}

int32 FFaerieItemContainerCountLimit::GetRemainingTotalItemCount() const
{
	if (MaxInstanceCount <= 0)
	{
		return ItemData::UnlimitedStack;
	}
	return MaxInstanceCount - CurrentTotalItemCopies;
}

bool FFaerieItemContainerCountLimit::CanContain(const int32 Count) const
{
	if (MaxEntryCount > 0)
	{
		// Maximum entries reached check
		if (EntryAmountCache.Num() >= MaxEntryCount)
		{
			return false;
		}
	}

	if (MaxInstanceCount > 0)
	{
		// Maximum total item reached check
		if (CurrentTotalItemCopies + Count > MaxInstanceCount)
		{
			return false;
		}
	}

	return true;
}

void FFaerieItemContainerCountLimit::UpdateCacheForEntry(const FFaerieEntryKey Key, const int32 Delta)
{
	int32 EntryAmount = 0;
	if (auto&& ExistingCache = EntryAmountCache.Find(Key))
	{
		EntryAmount = *ExistingCache;
	}

	EntryAmount += Delta;
	check(EntryAmount > 0);

	EntryAmountCache.Add(Key, EntryAmount);
	CurrentTotalItemCopies += Delta;
}

void FFaerieItemContainerCountLimit::RemoveCacheForEntry(const FFaerieEntryKey Key)
{
	if (const int32* ExistingCache = EntryAmountCache.Find(Key))
	{
		CurrentTotalItemCopies -= *ExistingCache;
		EntryAmountCache.Remove(Key);
	}
}

UFaerieItemContainerCountLimitUpdater::UFaerieItemContainerCountLimitUpdater()
  : EntityQuery(*this)
{
	// Process only on the server.
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieItemContainerCountLimitUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieItemContainerCountLimitUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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

			Container->WriteContainerData(FFaerieItemContainerCountLimit::StaticStruct(),
				[&Event](const FStructView Element)
				{
					auto& CountLimit = Element.Get<FFaerieItemContainerCountLimit>();
					CountLimit.PostEvent(Event);
				});
		}
	});
}

void FFaerieItemContainerPerEntryCountLimit::InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container)
{
	unimplemented();
}

EFaerieExtensionResponse FFaerieItemContainerPerEntryCountLimit::AllowsAddition(
	TNotNull<const UFaerieItemContainerBase*> Container, const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
	FFaerieExtensionAllowsAdditionArgs Args) const
{
	unimplemented();
	return EFaerieExtensionResponse::NoExplicitResponse;
}

void FFaerieItemContainerPerEntryCountLimit::SetPerEntryMaxInstanceCount(const int32 Count)
{
	PerEntryMaxInstanceCount = Count;
}

void FFaerieItemContainerPerEntryCountLimit::SetPerStackMaxCount(const int32 Count)
{
	PerStackMaxCount = Count;
}
