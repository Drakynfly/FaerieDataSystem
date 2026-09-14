// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/InventoryCapacityExtension.h"

#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryContentLog.h"

#include "MassExecutionContext.h"

#include "Capacity/FaerieCapacityHelper.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryCapacityExtension)

using namespace Faerie;

void FFaerieItemContainerCapacityData::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	for (auto It = Container::KeyRange(Container); It; ++It)
	{
		UpdateCacheForEntry(EntityManager, Container, *It);
	}
}

EFaerieExtensionResponse FFaerieItemContainerCapacityData::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container, const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
	FFaerieExtensionAllowsAdditionArgs Args) const
{
	// @todo Args.AddStackBehavior is not used at all.
	// Because CanContain doesnt check for Efficiency, there is no differance, but its technically incorrect.

	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
	if (Proxies.Num() == 1)
	{
		if (const FFaerieItemProxy Proxy0 = Proxies[0];
			!CanContain(EntityManager, Container, Proxy0))
		{
			const UFaerieItem* Item = Proxy0.GetItemInstanceOrInvalid().GetItemPtr();
			UE_LOGF(LogFaerieInventoryContent, Verbose, "AllowsAddition: Cannot add Stack (Item: '%ls' Copies: %i)",
				Item ? *Item->GetName() : TEXT("null"), Proxy0.GetCopies());
			return EFaerieExtensionResponse::Disallowed;
		}
		return EFaerieExtensionResponse::Allowed;
	}

	if (!CanContain_Multi(EntityManager, Container, Proxies))
	{
		UE_LOGF(LogFaerieInventoryContent, Verbose, "AllowsAddition: Cannot add Stacks in GroupTest");
		return EFaerieExtensionResponse::Disallowed;
	}

	return EFaerieExtensionResponse::Allowed;
}

bool FFaerieItemContainerCapacityData::HandleEvent(const FMassEntityManager& EntityManager, const Container::FEvent& Event)
{
	if (Event.EntryRemoved)
	{
		// Entry was removed, delete cache.
		return RemoveCacheForEntry(Event.Container.Get(), Event.EntryTouched);
	}

	return UpdateCacheForEntry(EntityManager, Event.Container.Get(), Event.EntryTouched);
}

bool FFaerieItemContainerCapacityData::CanContain(const FMassEntityManager& EntityManager,
	TNotNull<const UFaerieItemContainerBase*> Container, const TValid<const FFaerieItemProxy&> Proxy) const
{
	// @todo this does not account for the idea that if we add to an existing stack, the Efficiency would reduce the weight.

	const ItemData::FCapacityHelper Capacity(&EntityManager, ValidGet(Proxy).GetItemInstanceOrInvalid());

	// If the fragment is invalid, return true if we don't require one.
	if (!Capacity.HasCapacity())
	{
		return !Config.HasCheck(EFaerieCapacityExtensionChecks::Fragment);
	}

	// Determine if the entry cannot physically fit inside the dimensions of this container.
	// Fudged slightly to account for "cramming"
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Bounds))
	{
		// Convert Bounds to a FVector so we can multiply by a float, then convert back
		const FIntVector TestBounds = FIntVector(FVector(Config.Bounds) * Config.BoundsFudgeFactor);
		const FIntVector BoundsDiff = Capacity.GetCapacity().Bounds - TestBounds;

		// If the largest bound exceeds the limits, forbid containment.
		if (BoundsDiff.GetMax() > 0)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max weight.
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Weight))
	{
		const int32 TestWeight = State.CurrentWeight + Capacity.GetWeightOfStack(ValidGet(Proxy).GetCopies());
		const bool WouldExceedWeight = TestWeight > Config.MaxWeight;

		if (WouldExceedWeight)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max volume.
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Volume))
	{
		const int64 TestVolume = State.CurrentVolume + Capacity.GetVolumeOfStack(ValidGet(Proxy).GetCopies());
		const bool WouldExceedVolume = TestVolume > Config.MaxVolume;

		if (WouldExceedVolume)
		{
			return false;
		}
	}

	return true;
}

bool FFaerieItemContainerCapacityData::CanContain_Multi(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container,
	const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies) const
{
	// @todo this does not account for the idea that if we add to an existing stack, the Efficiency would reduce the weight.

	TArray<TUniquePtr<ItemData::FCapacityHelper>> Capacities;
	Capacities.Reserve(Proxies.Num());
	for (int32 i = 0; i < Proxies.Num(); ++i)
	{
		const FFaerieItemProxy Proxy = Proxies[i];
		if (!Proxy.IsValid())
		{
			return false;
		}

		TUniquePtr<ItemData::FCapacityHelper>& HelperPtr = Capacities.Add_GetRef(
			MakeUnique<ItemData::FCapacityHelper>(&EntityManager, Proxy.GetItemInstance().GetValue()));
		if (!HelperPtr->HasCapacity())
		{
			// If the fragment is invalid, return false if we require one.
			if (Config.HasCheck(EFaerieCapacityExtensionChecks::Fragment))
			{
				return false;
			}
		}
	}

	// Determine if the entry cannot physically fit inside the dimensions of this container.
	// Fudged slightly to account for "cramming"
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Bounds))
	{
		const FIntVector BoundsSum = [&Capacities]()
			{
				FIntVector Bounds;
				for (auto&& Capacity : Capacities)
				{
					Bounds += Capacity->GetCapacity().Bounds;
				}
				return Bounds;
			}();

		// Convert Bounds to a FVector so we can multiply by a float, then convert back
		const FIntVector TestBounds = FIntVector(FVector(Config.Bounds) * Config.BoundsFudgeFactor);
		const FIntVector BoundsDiff = BoundsSum - TestBounds;

		// If the largest bound exceeds the limits, forbid containment.
		if (BoundsDiff.GetMax() > 0)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max weight.
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Weight))
	{
		const int32 WeightsSum = [&Capacities, &Proxies]()
			{
				int32 Weights = 0;
				for (int32 i = 0; i < Proxies.Num(); ++i)
				{
					if (Capacities[i]->HasCapacity())
					{
						Weights += Capacities[i]->GetWeightOfStack(Proxies[i].GetCopies());
					}
				}

				return Weights;
			}();

		const int32 TestWeight = State.CurrentWeight + WeightsSum;
		const bool WouldExceedWeight = TestWeight > Config.MaxWeight;

		if (WouldExceedWeight)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max volume.
	if (Config.HasCheck(EFaerieCapacityExtensionChecks::Volume))
	{
		const int64 VolumesSum = [&Capacities, &Proxies]()
			{
				int64 Volumes = 0;
				for (int32 i = 0; i < Proxies.Num(); ++i)
				{
					if (Capacities[i]->HasCapacity())
					{
						Volumes += Capacities[i]->GetVolumeOfStack(Proxies[i].GetCopies());
					}
				}

				return Volumes;
			}();

		const int64 TestVolume = State.CurrentVolume + VolumesSum;
		const bool WouldExceedVolume = TestVolume > Config.MaxVolume;

		if (WouldExceedVolume)
		{
			return false;
		}
	}

	return true;
}

bool FFaerieItemContainerCapacityData::UpdateCacheForEntry(const FMassEntityManager& EntityManager, const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
{
	auto&& PrevCache = EntryCache.Find(Key);

	const ItemData::FScopeProxy View = Container->ViewEntry(Key);
	if (!View.IsValid())
	{
		UE_LOGF(LogFaerieInventoryContent, Error, "UpdateCacheForEntry should not handle removed entries!")

		if (PrevCache)
		{
			// Remove the existing cache by adding its inverse
			AddWeightAndVolume(-*PrevCache);
			EntryCache.Remove(Key);
			return true;
		}

		// We had nothing to do.
		return false;
	}

	FFaerieWeightAndVolume Total;

	// @todo we need to calculate the recursive capacity, not just the direct capacity.
	const ItemData::FCapacityHelper Capacity(&EntityManager, View.Instance);
	if (Capacity.HasCapacity())
	{
		// Get the weight of the sum of all stacks.
		Total.GramWeight = Capacity.GetWeightOfStack(View.Copies);

		// Calculate and add up the volumes of each stack.
		for (auto It = Container::SingleKeyRange(Container, Key); It; ++It)
		{
			Total.Volume += Capacity.GetVolumeOfStack(It.GetCopies());
		}
	}

	FFaerieWeightAndVolume Diff = Total;

	if (PrevCache)
	{
		Diff -= *PrevCache;
	}

	EntryCache.Add(Key, Total);
	AddWeightAndVolume(Diff);

	return true;
}

bool FFaerieItemContainerCapacityData::RemoveCacheForEntry(TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
{
	if (auto&& PrevCache = EntryCache.Find(Key))
	{
		// Remove the existing cache by adding its inverse
		AddWeightAndVolume(-*PrevCache);
		EntryCache.Remove(Key);
		return true;
	}
	return false;
}

void FFaerieItemContainerCapacityData::AddWeightAndVolume(const FFaerieWeightAndVolume Value)
{
	State.CurrentWeight += Value.GramWeight;
	State.CurrentVolume += Value.Volume;
}

void UFaerieItemContainerCapacityView::SyncView()
{
	if (!ContainerExtensionPtr.Key.IsValid())
	{
		return;
	}

	if (auto CapacityView = ContainerExtensionPtr.Value->Find(FFaerieItemContainerCapacityData::StaticStruct(), false);
		CapacityView.IsValid())
	{
		// @todo don't broadcast both, figure out what changed, either via equality check or pass changemask as parameter.
		auto& Capacity = CapacityView.Get<FFaerieItemContainerCapacityData>();
		Config = Capacity.GetConfig();
		State = Capacity.GetState();
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::Config);
		BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::State);
	}
}

void UFaerieItemContainerCapacityView::SetConfiguration(const FFaerieCapacityExtensionConfig& NewConfig)
{
	Config = NewConfig;

	if (Config.DeriveVolumeFromBounds)
	{
		Config.MaxVolume = Config.Bounds.X;
		Config.MaxVolume *= Config.Bounds.Y;
		Config.MaxVolume *= Config.Bounds.Z;
	}
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::Config);
}

void UFaerieItemContainerCapacityView::SetBounds(const FIntVector NewBounds)
{
	Config.Bounds = NewBounds;
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::Config);
}

void UFaerieItemContainerCapacityView::SetMaxCapacity(const FFaerieWeightAndVolume NewMax)
{
	Config.MaxWeight = NewMax.GramWeight;
	Config.MaxVolume = NewMax.Volume;
	BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::Config);
}

UFaerieItemContainerCapacityUpdater::UFaerieItemContainerCapacityUpdater()
  : EventQuery(*this), ViewQuery(*this)
{
	ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::AllNetModes);

	ExecutionOrder.ExecuteBefore.Add(Container::EventCleanup);

	// We write to container data and send events to game-thread UI
	bRequiresGameThreadExecution = true;
}

void UFaerieItemContainerCapacityUpdater::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EventQuery.AddRequirement<Container::FEvent>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	ViewQuery.AddRequirement<Content::FCapacityViewFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UFaerieItemContainerCapacityUpdater::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Track the containers we updated, so we can notify any active views of them.
	TSet<UObject*> ContainersUpdated;

	EventQuery.ForEachEntityChunk(Context, [this, &ContainersUpdated](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FEvent> Events = InContext.GetFragmentView<Container::FEvent>();
			for (const Container::FEvent& Event : Events)
			{
				UFaerieItemContainerBase* Container = Event.Container.Get();
				if (!IsValid(Container))
				{
					continue;
				}

				bool StateChanged = false;
				Container->WriteContainerData(FFaerieItemContainerCapacityData::StaticStruct(), [&InContext, &Event, &StateChanged](const FStructView Element)
				{
					auto& ContentHash = Element.Get<FFaerieItemContainerCapacityData>();
					StateChanged = ContentHash.HandleEvent(InContext.GetEntityManagerChecked(), Event);
				}, false);

				if (StateChanged)
				{
					ContainersUpdated.Add(Container);
				}
			}
		});

	ViewQuery.ForEachEntityChunk(Context, [&ContainersUpdated](const FMassExecutionContext& InContext)
		{
			const TConstArrayView<Container::FViewModelFragment> Views = InContext.GetFragmentView<Container::FViewModelFragment>(Content::FCapacityViewFragment::StaticStruct());
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