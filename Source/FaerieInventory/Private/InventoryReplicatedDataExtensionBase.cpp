// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryReplicatedDataExtensionBase.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemContainerBase.h"
#include "ItemContainerEvent.h"
#include "GameFramework/Actor.h"
#include "StructUtils/StructView.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "UObject/UObjectThreadContext.h"
#endif

#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryReplicatedDataExtensionBase)

void FFaerieReplicatedValue::PreReplicatedRemove(const FFaerieReplicatedSimMap& InArraySerializer)
{
	InArraySerializer.PreDataReplicatedRemove(*this);
}

void FFaerieReplicatedValue::PostReplicatedAdd(const FFaerieReplicatedSimMap& InArraySerializer)
{
	InArraySerializer.PostDataReplicatedAdd(*this);
}

void FFaerieReplicatedValue::PostReplicatedChange(const FFaerieReplicatedSimMap& InArraySerializer)
{
	InArraySerializer.PostDataReplicatedChange(*this);
}

void FFaerieReplicatedSimMap::RemoveValue(const FFaerieAddress Address)
{
	check(Address.IsValid());
	check(WriteLock == 0);

	if (Remove(Address,
		[this](const FFaerieReplicatedValue& Entry)
		{
			// Notify server of this removal.
			OwningWrapper->Server_PreContentRemoved(Entry);
		}))
	{
		// Notify clients of this removal.
		MarkArrayDirty();
	}
}

FInstancedStruct& FFaerieReplicatedSimMap::GetOrCreateValue(const FFaerieAddress Address)
{
	check(Address.IsValid());
	check(WriteLock == 0);

	// Find and return entry, if one exists
	if (const int32 Index = IndexOf(Address);
		Index != INDEX_NONE)
	{
		FFaerieReplicatedValue& EntryData = Entries[Index];
		MarkItemDirty(EntryData);

		// Notify server of this change.
		OwningWrapper->Server_PostContentChanged(EntryData);
		return EntryData.Value;
	}

	// Otherwise, make a new entry.
	FFaerieReplicatedValue& NewEntry = Insert(FFaerieReplicatedValue(Address,
		FInstancedStruct(OwningWrapper->GetOuterUInventoryReplicatedDataExtensionBase()->GetDataScriptStruct())));
	MarkItemDirty(NewEntry);

	// Notify server of this change.
	OwningWrapper->Server_PostContentAdded(NewEntry);
	return NewEntry.Value;
}

void FFaerieReplicatedSimMap::SetValue(const FFaerieAddress Address, const FInstancedStruct& Data)
{
	check(Address.IsValid());
	check(WriteLock == 0);

	if (const int32 Index = IndexOf(Address);
		Index != INDEX_NONE)
	{
		FFaerieReplicatedValue& EntryData = Entries[Index];
		EntryData.Value = Data;
		MarkItemDirty(EntryData);

		// Notify server of this change.
		OwningWrapper->Server_PostContentChanged(EntryData);
	}
	else
	{
		FFaerieReplicatedValue& NewEntry = Insert(FFaerieReplicatedValue(Address, Data));
		MarkItemDirty(NewEntry);

		// Notify server of this change.
		OwningWrapper->Server_PostContentAdded(NewEntry);
	}
}

FFaerieReplicatedSimMap::FValueWriteScope::FValueWriteScope(const FFaerieAddress Address, FFaerieReplicatedSimMap& Source)
  : Handle(Source.Entries[Source.IndexOf(Address)]),
	Source(Source)
{
	Source.WriteLock++;
}

FFaerieReplicatedSimMap::FValueWriteScope::~FValueWriteScope()
{
	Source.WriteLock--;

	// Propagate change to client
	Source.MarkItemDirty(Handle);

	// Broadcast change on server
	Source.OwningWrapper->Server_PostContentChanged(Handle);
}

void FFaerieReplicatedSimMap::PreDataReplicatedRemove(const FFaerieReplicatedValue& Data) const
{
	if (IsValid(OwningWrapper))
	{
		OwningWrapper->Client_PreContentRemoved(Data);
	}
}

void FFaerieReplicatedSimMap::PostDataReplicatedAdd(const FFaerieReplicatedValue& Data) const
{
	if (IsValid(OwningWrapper))
	{
		OwningWrapper->Client_PostContentAdded(Data);
	}
}

void FFaerieReplicatedSimMap::PostDataReplicatedChange(const FFaerieReplicatedValue& Data) const
{
	if (IsValid(OwningWrapper))
	{
		OwningWrapper->Client_PostContentChanged(Data);
	}
}

FFaerieReplicatedSimMap::TRangedForConstIterator FFaerieReplicatedSimMap::begin() const
{
	WriteLock++;
	return TRangedForConstIterator(Entries.begin());
}

FFaerieReplicatedSimMap::TRangedForConstIterator FFaerieReplicatedSimMap::end() const
{
	WriteLock--;
	return TRangedForConstIterator(Entries.end());
}

void URepDataArrayWrapper::PostInitProperties()
{
	Super::PostInitProperties();

	// Bind replication functions out into this class.
	DataArray.OwningWrapper = this;
}

void URepDataArrayWrapper::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Container, COND_InitialOnly);
	DOREPLIFETIME(ThisClass, DataArray);
}

void URepDataArrayWrapper::Server_PreContentRemoved(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataRemoved(Container.Get(), Data);
	}
}

void URepDataArrayWrapper::Server_PostContentAdded(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataAdded(Container.Get(), Data);
	}
}

void URepDataArrayWrapper::Server_PostContentChanged(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataChanged(Container.Get(), Data);
	}
}

void URepDataArrayWrapper::Client_PreContentRemoved(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataRemoved(Container.Get(), Data);
	}
}

void URepDataArrayWrapper::Client_PostContentAdded(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataAdded(Container.Get(), Data);
	}
}

void URepDataArrayWrapper::Client_PostContentChanged(const FFaerieReplicatedValue& Data)
{
	if (Container.IsValid())
	{
		GetOuterUInventoryReplicatedDataExtensionBase()->PreEntryDataChanged(Container.Get(), Data);
	}
}

void UInventoryReplicatedDataExtensionBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PerContainerData, SharedParams)
}

FInstancedStruct UInventoryReplicatedDataExtensionBase::MakeSaveData(const UFaerieItemContainerBase* Container) const
{
	if (SaveRepDataArray())
	{
		return FInstancedStruct(FindFastArrayForContainer(Container));
	}

	return FInstancedStruct();
}

void UInventoryReplicatedDataExtensionBase::LoadSaveData(const UFaerieItemContainerBase* Container,
	const FInstancedStruct& SaveData)
{
	if (const TStructView<FFaerieReplicatedSimMap> ContainerData = FindFastArrayForContainer(Container);
		ContainerData.IsValid())
	{
		FFaerieReplicatedSimMap& Ref = ContainerData.Get<FFaerieReplicatedSimMap>();
		Ref.Entries = SaveData.Get<FFaerieReplicatedSimMap>().Entries;
		Ref.MarkArrayDirty();
	}
}

void UInventoryReplicatedDataExtensionBase::InitializeExtension(const UFaerieItemContainerBase* Container)
{
#if WITH_EDITOR
	checkf(FUObjectThreadContext::Get().IsInConstructor == false, TEXT("Do not call InitializeExtension from a constructor! Use InitializeNetObject if available."));

	// Explanation: For UFaerieInventoryComponents that are added to Blueprint Classes, sometimes in PIE, the
	// component's GEN_VARIABLE version will somehow slip through and try to be added as a container. They are invalid.
	if (Container->GetFullName().Contains("GEN_VARIABLE"))
	{
		//ensure(0);
		return;
	}
#endif

	checkSlow(!FindFastArrayForContainer(Container).IsValid());

	if (ensure(IsValid(Container)))
	{
		URepDataArrayWrapper* NewWrapper = NewObject<URepDataArrayWrapper>(this);
		NewWrapper->Container = Container;

		if (AActor* Actor = GetTypedOuter<AActor>();
			IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
		{
			Actor->AddReplicatedSubObject(NewWrapper);
			NewWrapper->InitializeNetObject(Actor);
		}

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PerContainerData, this);
		PerContainerData.Add(NewWrapper);
	}
}

void UInventoryReplicatedDataExtensionBase::DeinitializeExtension(const UFaerieItemContainerBase* Container)
{
	if (!!PerContainerData.RemoveAll(
		[this, Container](const TObjectPtr<URepDataArrayWrapper>& Wrapper)
		{
			if (Wrapper->Container == Container)
			{
				if (AActor* Actor = GetTypedOuter<AActor>();
				IsValid(Actor) && Actor->IsUsingRegisteredSubObjectList())
				{
					Actor->RemoveReplicatedSubObject(Wrapper);
					Wrapper->DeinitializeNetObject(Actor);
				}
				return true;
			}
			return false;
		}))
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PerContainerData, this);
	}
}

/*
void UInventoryReplicatedDataExtensionBase::PreRemoval(const UFaerieItemContainerBase* Container, const FEntryKey Key,
													   const int32 Removal)
{
	Super::PreRemoval(Container, Key, Removal);

	if (const TStructView<FFaerieReplicatedSimMap> ContainerData = FindFastArrayForContainer(Container);
		ContainerData.IsValid())
	{
		FFaerieReplicatedSimMap& Ref = ContainerData.Get<FFaerieReplicatedSimMap>();

		// If the whole stack is being removed, auto-delete any data we have for the entry
		if (Container->GetStack(Key) == Removal || Removal == Faerie::ItemData::EntireStack)
		{
			Ref.RemoveValue(Key);
		}
	}
}
*/

void UInventoryReplicatedDataExtensionBase::PostRemoval(const UFaerieItemContainerBase* Container,
	const Faerie::Inventory::FEventLog& Event)
{
	Super::PostRemoval(Container, Event);

	using namespace Faerie::Container;

	if (const TStructView<FFaerieReplicatedSimMap> ContainerData = FindFastArrayForContainer(Container);
		ContainerData.IsValid())
	{
		FFaerieReplicatedSimMap& Ref = ContainerData.Get<FFaerieReplicatedSimMap>();

		for (const FFaerieAddress Address : KeyFilter(Container)
				.Run(FSingleKey(Event.EntryTouched))
				.AddressRange())
		{
			// If the whole stack was removed, delete any data we have for the entry
			if (!Container->Contains(Address))
			{
				Ref.RemoveValue(Address);
			}
		}
	}
}

FConstStructView UInventoryReplicatedDataExtensionBase::GetDataForHandle(const FFaerieAddressableHandle Handle) const
{
	if (const TConstStructView<FFaerieReplicatedSimMap> ContainerData = FindFastArrayForContainer(Handle.Container.Get());
		ensure(ContainerData.IsValid()))
	{
		const FFaerieReplicatedSimMap& Ref = ContainerData.Get();

		if (const FFaerieReplicatedValue* Element = Ref.Find(Handle.Address))
		{
			return Element->Value;
		}
	}
	return FConstStructView();
}

bool UInventoryReplicatedDataExtensionBase::EditDataForHandle(const FFaerieAddressableHandle Handle,
															  const TFunctionRef<void(FStructView)>& Edit)
{
	const TStructView<FFaerieReplicatedSimMap> ContainerData = FindFastArrayForContainer(Handle.Container.Get());
	if (!ContainerData.IsValid())
	{
		return false;
	}

	FFaerieReplicatedSimMap& Ref = ContainerData.Get<FFaerieReplicatedSimMap>();

	// Find and use entry, if one exists
	if (const int32 Index = Ref.IndexOf(Handle.Address);
		Index != INDEX_NONE)
	{
		const FFaerieReplicatedSimMap::FValueWriteScope Scope = Ref.GetWriteScope(Handle.Address);
		Edit(Scope.Get());
		return true;
	}

	// Otherwise, make a new entry.
	FFaerieReplicatedValue& NewEntry = Ref.Insert(FFaerieReplicatedValue(Handle.Address, FInstancedStruct(GetDataScriptStruct())));

	Edit(NewEntry.Value);
	Ref.MarkItemDirty(NewEntry);

	// Notify server of this change.
	Ref.OwningWrapper->Server_PostContentAdded(NewEntry);

	return true;
}

TStructView<FFaerieReplicatedSimMap> UInventoryReplicatedDataExtensionBase::FindFastArrayForContainer(const UFaerieItemContainerBase* Container)
{
	if (auto&& Found = PerContainerData.FindByPredicate(
			[Container](const TObjectPtr<URepDataArrayWrapper>& Data)
			{
				return Data && Data->Container == Container;
			}))
	{
		return (*Found)->DataArray;
	}
#if WITH_EDITOR
	UE_LOG(LogFaerieInventory, Warning, TEXT("Failed to find FastArray for container '%s'. Is this container initialized to this extension?"), *Container->GetFullName())
	PrintPerContainerDataDebug();
#endif
	return TStructView<FFaerieReplicatedSimMap>();
}

TConstStructView<FFaerieReplicatedSimMap> UInventoryReplicatedDataExtensionBase::FindFastArrayForContainer(const UFaerieItemContainerBase* Container) const
{
	if (auto&& Found = PerContainerData.FindByPredicate(
			[Container](const TObjectPtr<URepDataArrayWrapper>& Data)
			{
				return Data && Data->Container == Container;
			}))
	{
		return (*Found)->DataArray;
	}
#if WITH_EDITOR
	UE_LOG(LogFaerieInventory, Warning, TEXT("Failed to find FastArray for container '%s'. Is this container initialized to this extension?"), *Container->GetFullName())
	PrintPerContainerDataDebug();
#endif
	return TConstStructView<FFaerieReplicatedSimMap>();
}

#if WITH_EDITOR
void UInventoryReplicatedDataExtensionBase::PrintPerContainerDataDebug() const
{
	UE_LOG(LogFaerieInventory, Log, TEXT("Printing Containers with FastArrays"))
	for (auto&& Element : PerContainerData)
	{
		UE_LOG(LogFaerieInventory, Log, TEXT("    Data: '%s' - Container: '%s'"), *Element->GetFullName(), *Element->Container->GetFullName())
	}
}
#endif