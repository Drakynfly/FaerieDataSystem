// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/InventoryCapacityExtension.h"

#include "EntityManagerHelpers.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryContentLog.h"

#include "ItemContainerEvent.h"

#include "Capacity/FaerieCapacityHelper.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryCapacityExtension)

using namespace Faerie;

void UInventoryCapacityExtension::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Config, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, State, SharedParams)
}

#if WITH_EDITOR
void UInventoryCapacityExtension::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Config.DeriveVolumeFromBounds)
	{
		Config.MaxVolume = Config.Bounds.X;
		Config.MaxVolume *= Config.Bounds.Y;
		Config.MaxVolume *= Config.Bounds.Z;
	}
}

void UInventoryCapacityExtension::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (Config.DeriveVolumeFromBounds)
	{
		Config.MaxVolume = Config.Bounds.X;
		Config.MaxVolume *= Config.Bounds.Y;
		Config.MaxVolume *= Config.Bounds.Z;
	}
}
#endif

void UInventoryCapacityExtension::InitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	for (auto It = Container::KeyRange(Container); It; ++It)
	{
		UpdateCacheForEntry(Container, *It);
	}

	HandleStateChanged();
}

void UInventoryCapacityExtension::DeinitializeExtension(const TNotNull<const UFaerieItemContainerBase*> Container)
{
	if (!ServerCapacityCache.Contains(Container)) return;

	for (auto&& Cache = ServerCapacityCache[Container];
		auto&& Element : Cache)
	{
		// Remove the existing cache by adding its inverse
		AddWeightAndVolume(-Element.Value);
	}

	ServerCapacityCache.Remove(Container);

	HandleStateChanged();
}

EEventExtensionResponse UInventoryCapacityExtension::AllowsAddition(const TNotNull<const UFaerieItemContainerBase*> Container,
																	const TConstArrayView<FFaerieItemDataView> Views,
																	const FFaerieExtensionAllowsAdditionArgs Args) const
{
	// @todo Args.AddStackBehavior is not used at all.
	// Because CanContain doesnt check for Efficiency, there is no differance, but its technically incorrect.

	if (Views.Num() == 1)
	{
		if (const FFaerieItemDataView View0 = Views[0];
			!CanContain(View0))
		{
			const UFaerieItem* Item = View0.GetInstance().GetItemPtr();
			UE_LOG(LogFaerieInventoryContent, Verbose, TEXT("PreAddition: Cannot add Stack (Item: '%s' Copies: %i)"),
				Item ? *Item->GetName() : TEXT("null"), View0.GetCopies());
			return EEventExtensionResponse::Disallowed;
		}
		return EEventExtensionResponse::Allowed;
	}

	switch (Args.TestType)
	{
	case EFaerieStorageAddStackTestMultiType::IndividualTests:
		{
			for (const FFaerieItemDataView& View : Views)
			{
				if (!CanContain(View))
				{
					const UFaerieItem* Item = View.GetInstance().GetItemPtr();
					UE_LOG(LogFaerieInventoryContent, Verbose, TEXT("PreAddition: Cannot add Stack (Item: '%s' Copies: %i)"),
						Item ? *Item->GetName() : TEXT("null"), View.GetCopies());
					return EEventExtensionResponse::Disallowed;
				}
			}
			return EEventExtensionResponse::Allowed;
		}

	case EFaerieStorageAddStackTestMultiType::GroupTest:
		{
			if (!CanContain_Multi(Views))
			{
				UE_LOG(LogFaerieInventoryContent, Verbose, TEXT("PreAddition: Cannot add Stacks in GroupTest"));
				return EEventExtensionResponse::Disallowed;
			}
		}
		return EEventExtensionResponse::Allowed;
	}

	// Should not reach this;
	return EEventExtensionResponse::NoExplicitResponse;
}

void UInventoryCapacityExtension::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Inventory::FEventLogBatch& Events)
{
	for (auto&& Event : Events.Data)
	{
		UpdateCacheForEntry(Container, Event.EntryTouched);
	}
	HandleStateChanged();
}

void UInventoryCapacityExtension::UpdateCacheForEntry(const TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieEntryKey Key)
{
	auto&& ContainerCache = ServerCapacityCache.FindOrAdd(Container);
	auto&& PrevCache = ContainerCache.Find(Key);

	const FFaerieItemDataView View = Container->ViewEntry(Key);
	if (!View.IsValid())
	{
		if (PrevCache)
		{
			// Remove the existing cache by adding its inverse
			AddWeightAndVolume(-*PrevCache);
			ContainerCache.Remove(Key);
		}
		return;
	}

	const FFaerieWeightAndVolume Total = [&]()
		{
			FFaerieWeightAndVolume Out;

			const ItemData::FCapacityHelper Capacity(ItemData::FOptionalEntityManager(this), View.GetInstance());
			if (!Capacity.HasCapacity())
			{
				return Out;
			}

			Out.GramWeight = Capacity.GetWeightOfStack(View.GetCopies());

			for (auto It = Container::SingleKeyRange(Container, Key); It; ++It)
			{
				Out.Volume += Capacity.GetVolumeOfStack(It.GetCopies());
			}

			return Out;
		}();

	FFaerieWeightAndVolume Diff = Total;

	if (PrevCache)
	{
		Diff -= *PrevCache;
	}

	ContainerCache.Add(Key, Total);
	AddWeightAndVolume(Diff);
}

void UInventoryCapacityExtension::CheckCapacityLimit()
{
	const bool IsExceedingWeight = State.CurrentWeight > Config.MaxWeight;
	const bool IsExceedingVolume = State.CurrentVolume > Config.MaxVolume;

	if (IsExceedingWeight != State.OverMaxWeight)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, State, this);
		State.OverMaxWeight = IsExceedingWeight;
	}
	if (IsExceedingVolume != State.OverMaxVolume)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, State, this);
		State.OverMaxVolume = IsExceedingVolume;
	}
}

bool UInventoryCapacityExtension::CanContainItem(const ItemData::FValidatedDataView& View) const
{
	// @todo this does not account for the idea that if we add to an existing stack, the Efficiency would reduce the weight.

	const ItemData::FCapacityHelper Capacity(ItemData::FOptionalEntityManager(this), View->GetInstance());

	// If the fragment is invalid, return true if we don't require one.
	if (!Capacity.HasCapacity())
	{
		return !Config.HasCheck(ECapacityChecks::Fragment);
	}

	// Determine if the entry cannot physically fit inside the dimensions of this container.
	// Fudged slightly to account for "cramming"
	if (Config.HasCheck(ECapacityChecks::Bounds))
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
	if (Config.HasCheck(ECapacityChecks::Weight))
	{
		const int32 TestWeight = State.CurrentWeight + Capacity.GetWeightOfStack(View->GetCopies());
		const bool WouldExceedWeight = TestWeight > Config.MaxWeight;

		if (WouldExceedWeight)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max volume.
	if (Config.HasCheck(ECapacityChecks::Volume))
	{
		const int64 TestVolume = State.CurrentVolume + Capacity.GetVolumeOfStack(View->GetCopies());
		const bool WouldExceedVolume = TestVolume > Config.MaxVolume;

		if (WouldExceedVolume)
		{
			return false;
		}
	}

	return true;
}

void UInventoryCapacityExtension::AddWeightAndVolume(const FFaerieWeightAndVolume Value)
{
	if (Value.IsInsignificant()) return;

	State.CurrentWeight += Value.GramWeight;
	State.CurrentVolume += Value.Volume;
}

void UInventoryCapacityExtension::HandleStateChanged()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, State, this);
	CheckCapacityLimit();
	OnStateChangedNative.Broadcast();
	OnStateChanged.Broadcast();
}

bool UInventoryCapacityExtension::CanContain(const FFaerieItemDataView& View) const
{
	if (!View.IsValid())
	{
		return false;
	}

	return CanContainItem(View);
}

bool UInventoryCapacityExtension::CanContain_Multi(const TConstArrayView<FFaerieItemDataView> Views) const
{
	// @todo this does not account for the idea that if we add to an existing stack, the Efficiency would reduce the weight.

	const ItemData::FOptionalEntityManager EntityManager(this);
	TArray<TUniquePtr<ItemData::FCapacityHelper>> Capacities;
	Capacities.Reserve(Views.Num());
	for (auto&& View : Views)
	{
		if (!View.IsValid())
		{
			return false;
		}

		TUniquePtr<ItemData::FCapacityHelper>& HelperPtr = Capacities.Add_GetRef(
			MakeUnique<ItemData::FCapacityHelper>(EntityManager, View.GetInstance()));
		if (!HelperPtr->HasCapacity())
		{
			// If the fragment is invalid, return false if we require one.
			if (Config.HasCheck(ECapacityChecks::Fragment))
			{
				return false;
			}
		}
	}

	// Determine if the entry cannot physically fit inside the dimensions of this container.
	// Fudged slightly to account for "cramming"
	if (Config.HasCheck(ECapacityChecks::Bounds))
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
	if (Config.HasCheck(ECapacityChecks::Weight))
	{
		const int32 WeightsSum = [&Capacities, &Views]()
			{
				int32 Weights = 0;
				for (int32 i = 0; i < Views.Num(); ++i)
				{
					if (Capacities[i]->HasCapacity())
					{
						Weights += Capacities[i]->GetWeightOfStack(Views[i].GetCopies());
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
	if (Config.HasCheck(ECapacityChecks::Volume))
	{
		const int64 VolumesSum = [&Capacities, &Views]()
			{
				int64 Volumes = 0;
				for (int32 i = 0; i < Views.Num(); ++i)
				{
					if (Capacities[i]->HasCapacity())
					{
						Volumes += Capacities[i]->GetVolumeOfStack(Views[i].GetCopies());
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

bool UInventoryCapacityExtension::CanContainProxy(const FFaerieItemProxy& Proxy) const
{
	if (!ensure(Proxy.IsValid()))
	{
		return false;
	}

	return CanContainItem(FFaerieItemDataView(Proxy));
}

FFaerieWeightAndVolume UInventoryCapacityExtension::GetCurrentCapacity() const
{
    return FFaerieWeightAndVolume(State.CurrentWeight, State.CurrentVolume);
}

FFaerieWeightAndVolume UInventoryCapacityExtension::GetMaxCapacity() const
{
    return FFaerieWeightAndVolume(Config.MaxWeight, Config.MaxVolume);
}

void UInventoryCapacityExtension::SetConfiguration(const FCapacityExtensionConfig& NewConfig)
{
	Config = NewConfig;

	if (Config.DeriveVolumeFromBounds)
	{
		Config.MaxVolume = Config.Bounds.X;
		Config.MaxVolume *= Config.Bounds.Y;
		Config.MaxVolume *= Config.Bounds.Z;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Config, this);
	CheckCapacityLimit();
	OnConfigurationChangedNative.Broadcast();
	OnConfigurationChanged.Broadcast();
}

void UInventoryCapacityExtension::SetBounds(const FIntVector NewBounds)
{
	Config.Bounds = NewBounds;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Config, this);
	CheckCapacityLimit();
	OnConfigurationChangedNative.Broadcast();
	OnConfigurationChanged.Broadcast();
}

void UInventoryCapacityExtension::SetMaxCapacity(const FFaerieWeightAndVolume NewMax)
{
	Config.MaxWeight = NewMax.GramWeight;
	Config.MaxVolume = NewMax.Volume;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Config, this);
	CheckCapacityLimit();
	OnConfigurationChangedNative.Broadcast();
	OnConfigurationChanged.Broadcast();
}

float UInventoryCapacityExtension::GetPercentageFullForWeightAndVolume(const FFaerieWeightAndVolume& WeightAndVolume) const
{
	float ScalarWeightFull = 0;
	float ScalarVolumeFull = 0;

	if (Config.MaxWeight > 0)
	{
		ScalarWeightFull = static_cast<float>(WeightAndVolume.GramWeight) / static_cast<float>(Config.MaxWeight);
	}

	if (Config.MaxVolume > 0)
	{
		ScalarVolumeFull = static_cast<float>(WeightAndVolume.Volume) / static_cast<float>(Config.MaxVolume);
	}

	const float LargerFull = FMath::Max(ScalarWeightFull, ScalarVolumeFull);
	const float SmallerFull = FMath::Min(ScalarWeightFull, ScalarVolumeFull);

	const float SecondAmountToFill = 1 - LargerFull;
	return LargerFull + (SmallerFull * SecondAmountToFill);
}

float UInventoryCapacityExtension::GetPercentageFull() const
{
	return GetPercentageFullForWeightAndVolume(GetCurrentCapacity());
}

void UInventoryCapacityExtension::OnRep_Config()
{
	OnConfigurationChangedNative.Broadcast();
	OnConfigurationChanged.Broadcast();
}

void UInventoryCapacityExtension::OnRep_State()
{
	OnStateChangedNative.Broadcast();
	OnStateChanged.Broadcast();
}