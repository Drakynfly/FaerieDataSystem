﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryCapacityExtension.h"

#include "FaerieItem.h"
#include "FaerieItemStorage.h"
#include "Net/UnrealNetwork.h"
#include "Tokens/FaerieCapacityToken.h"

void UInventoryCapacityExtension::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Config, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, State, SharedParams);
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

void UInventoryCapacityExtension::InitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

	Container->ForEachKey(
		[this, Container](const FEntryKey Key)
		{
			UpdateCacheForEntry(Container, Key);
		});

	HandleStateChanged();
}

void UInventoryCapacityExtension::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!ensure(IsValid(Container))) return;

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

EEventExtensionResponse UInventoryCapacityExtension::AllowsAddition(const UFaerieItemContainerBase* Container, const FFaerieItemStackView Stack)
{
	if (!CanContain(Stack))
	{
		UE_LOG(LogTemp, Warning, TEXT("PreAddition: Cannot add Stack (Item: '%s' Copies: %i)"), *Stack.Item.GetName(), Stack.Copies);
		return EEventExtensionResponse::Disallowed;
	}

	return EEventExtensionResponse::Allowed;
}

void UInventoryCapacityExtension::PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	UpdateCacheForEntry(Container, Event.EntryTouched);
	HandleStateChanged();
}

void UInventoryCapacityExtension::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	UpdateCacheForEntry(Container, Event.EntryTouched);
	HandleStateChanged();
}

void UInventoryCapacityExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	UpdateCacheForEntry(Container, Key);
	HandleStateChanged();
}

FWeightAndVolume UInventoryCapacityExtension::GetEntryWeightAndVolume(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	FWeightAndVolume Out;

	const FFaerieItemStackView View = Container->View(Key);
	if (!IsValid(View.Item))
	{
		return Out;
	}

	auto&& Token = View.Item->GetToken<UFaerieCapacityToken>();
	if (!IsValid(Token))
	{
		return Out;
	}

	Out.GramWeight = Token->GetWeightOfStack(View.Copies);

	if (auto&& AsStorage = Cast<UFaerieItemStorage>(Container))
	{
		// @todo a nicer way to do this would be ideal. but since UFaerieItemStorage has custom stacking logic, we
		// have to sum it seperately to handle efficiency per stack correctly
		const FConstStructView EntryView = AsStorage->GetEntryView(Key);
		if (!ensure(EntryView.IsValid()))
		{
			return Out;
		}

		for (const FInventoryEntry& Entry = EntryView.Get<const FInventoryEntry>();
			auto&& KeyedStack : Entry.Stacks)
		{
			Out.Volume += Token->GetVolumeOfStack(KeyedStack.Stack);
		}
	}
	else
	{
		// Other container types (like EquipmentSlot) fallback to the default logic
		Out.Volume += Token->GetVolumeOfStack(View.Copies);
	}

	return Out;
}

void UInventoryCapacityExtension::UpdateCacheForEntry(const UFaerieItemContainerBase* Container, const FEntryKey Key)
{
	if (!ensure(IsValid(Container))) return;

	auto&& ContainerCache = ServerCapacityCache.FindOrAdd(Container);
	auto&& PrevCache = ContainerCache.Find(Key);

	if (!Container->IsValidKey(Key))
	{
		if (PrevCache)
		{
			// Remove the existing cache by adding its inverse
			AddWeightAndVolume(-*PrevCache);
			ContainerCache.Remove(Key);
		}
		return;
	}

	auto&& Total = GetEntryWeightAndVolume(Container, Key);

	FWeightAndVolume Diff = Total;

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

bool UInventoryCapacityExtension::CanContainToken(const UFaerieCapacityToken* Token, const int32 Stack) const
{
	// We can always contain items with no capacity requirement.
	if (!IsValid(Token))
	{
		return true;
	}

	// Determine if the entry cannot physically fit inside the dimensions of this container.
	// Fudged slightly to account for "cramming"
	if (Config.HasCheck(ECapacityChecks::Bounds))
	{
		// Convert Bounds to a FVector so we can multiply by a float, then convert back
		const FIntVector TestBounds = FIntVector(FVector(Config.Bounds) * Config.BoundsFudgeFactor);
		const FIntVector BoundsDiff = Token->GetCapacity().Bounds - TestBounds;

		// If the largest bound exceeds the limits, forbid containment.
		if (BoundsDiff.GetMax() > 0)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max weight.
	if (Config.HasCheck(ECapacityChecks::Weight))
	{
		const int32 TestWeight = State.CurrentWeight + Token->GetWeightOfStack(Stack);
		const bool WouldExceedWeight = TestWeight > Config.MaxWeight;

		if (WouldExceedWeight)
		{
			return false;
		}
	}

	// Determine if the entry would put the container over max volume.
	if (Config.HasCheck(ECapacityChecks::Volume))
	{
		const int64 TestVolume = State.CurrentVolume + Token->GetVolumeOfStack(Stack);
		const bool WouldExceedVolume = TestVolume > Config.MaxVolume;

		if (WouldExceedVolume)
		{
			return false;
		}
	}

	return true;
}

void UInventoryCapacityExtension::AddWeightAndVolume(const FWeightAndVolume Value)
{
	if (Value.IsInsignificant()) return;

	State.CurrentWeight += Value.GramWeight;
	State.CurrentVolume += Value.Volume;
}

void UInventoryCapacityExtension::HandleStateChanged()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, State, this);
	CheckCapacityLimit();
	OnStateChanged.Broadcast();
}

bool UInventoryCapacityExtension::CanContain(const FFaerieItemStackView Stack) const
{
	auto&& CapacityToken = Stack.Item->GetToken<UFaerieCapacityToken>();

	if (!IsValid(CapacityToken))
	{
		return Config.AllowEntriesWithNoCapacityToken;
	}

	return CanContainToken(CapacityToken, Stack.Copies);
}

bool UInventoryCapacityExtension::CanContainProxy(const FFaerieItemProxy Proxy) const
{
	if (!ensure(Proxy.IsValid()))
	{
		return false;
	}

	auto&& ItemObject = Proxy->GetItemObject();

	if (!ensure(IsValid(ItemObject)))
	{
		return false;
	}

	auto&& CapacityToken = ItemObject->GetToken<UFaerieCapacityToken>();

	if (!IsValid(CapacityToken))
	{
		return Config.AllowEntriesWithNoCapacityToken;
	}

	const int32 Stack = Proxy->GetCopies();

	if (!Faerie::ItemData::IsValidStack(Stack))
	{
		return false;
	}

	return CanContainToken(CapacityToken, Stack);
}

FWeightAndVolume UInventoryCapacityExtension::GetCurrentCapacity() const
{
    return FWeightAndVolume(State.CurrentWeight, State.CurrentVolume);
}

FWeightAndVolume UInventoryCapacityExtension::GetMaxCapacity() const
{
    return FWeightAndVolume(Config.MaxWeight, Config.MaxVolume);
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
	OnConfigurationChanged.Broadcast();
}

void UInventoryCapacityExtension::SetMaxCapacity(const FWeightAndVolume NewMax)
{
	Config.MaxWeight = NewMax.GramWeight;
	Config.MaxVolume = NewMax.Volume;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Config, this);
	CheckCapacityLimit();
	OnConfigurationChanged.Broadcast();
}

float UInventoryCapacityExtension::GetPercentageFullForWeightAndVolume(const FWeightAndVolume& WeightAndVolume) const
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
	OnConfigurationChanged.Broadcast();
}

void UInventoryCapacityExtension::OnRep_State()
{
	OnStateChanged.Broadcast();
}