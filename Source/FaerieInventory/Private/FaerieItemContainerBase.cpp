// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerBase.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"
#include "FaerieContainerFilter.h"
#include "FaerieItem.h"
#include "FaerieSubObjectFilter.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"
#include "MassCommandBuffer.h"

#include "Actions/FaerieInventoryClient.h"

#include "Fragments/ContainerMetadataFragment.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerBase)

using namespace Faerie;

void FFaerieItemContainerExtensions::PreStackReplicatedRemove(
	const FFaerieItemContainerExtensionStorageElement& Element) const
{
}

void FFaerieItemContainerExtensions::PostStackReplicatedAdd(
	const FFaerieItemContainerExtensionStorageElement& Element) const
{
}

void FFaerieItemContainerExtensions::PostStackReplicatedChange(
	const FFaerieItemContainerExtensionStorageElement& Element) const
{
}

FConstStructView FFaerieItemContainerExtensions::Find(const TNotNull<const UScriptStruct*> Type, const bool RecurseParents) const
{
	for (const FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (Item.ExtensionStruct.GetScriptStruct() == Type)
		{
			return Item.ExtensionStruct;
		}
	}

	if (RecurseParents)
	{
		if (ParentExtensions.Key.IsValid())
		{
			return ParentExtensions.Value->Find(Type, true);
		}
	}

	return FConstStructView();
}

FStructView FFaerieItemContainerExtensions::Find(const TNotNull<const UScriptStruct*> Type, const bool RecurseParents)
{
	for (FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (Item.ExtensionStruct.GetScriptStruct() == Type)
		{
			return Item.ExtensionStruct;
		}
	}

	if (RecurseParents)
	{
		if (ParentExtensions.Key.IsValid())
		{
			return ParentExtensions.Value->Find(Type, true);
		}
	}

	return FStructView();
}

FInstancedStruct& FFaerieItemContainerExtensions::AddOrGetRef(const TNotNull<const UScriptStruct*> Type, EWriteContainerDataFlag* OutFlag)
{
	for (auto&& Element : Items)
	{
		if (Element.ExtensionStruct.GetScriptStruct() == Type)
		{
			MarkItemDirty(Element);
			if (OutFlag)
			{
				*OutFlag = None;
			}
			return Element.ExtensionStruct;
		}
	}

	auto& NewElement = Items.AddDefaulted_GetRef();
	NewElement.ExtensionStruct.InitializeAs(Type);
	if (auto Extension = NewElement.ExtensionStruct.GetMutablePtr<FFaerieItemContainerExtensionBase>())
	{
		Extension->InitializeExtension(ChangeListener);
	}
	MarkItemDirty(NewElement);
	if (OutFlag)
	{
		*OutFlag = Created;
	}
	return NewElement.ExtensionStruct;
}

void FFaerieItemContainerExtensions::Remove(const int32 Index)
{
	Items.RemoveAt(Index);

	// Notify clients of this removal.
	MarkArrayDirty();
}

void FFaerieItemContainerExtensions::Reset()
{
	Items.Empty();
	MarkArrayDirty();
}

void FFaerieItemContainerExtensions::ForEach(const TFunctionRef<Utils::EIteratorFunctorReturn(FConstStructView)>& Functor) const
{
	for (const FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (!Item.ExtensionStruct.IsValid()) continue;

		switch (Functor(Item.ExtensionStruct))
		{
		case Utils::Break:
			// Exit loop
			return;
		case Utils::Continue:
			break;
		}
	}
}

void FFaerieItemContainerExtensions::ForEachMutable(const TFunctionRef<Utils::EIteratorFunctorReturn(FStructView)>& Functor)
{
	for (FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (!Item.ExtensionStruct.IsValid()) continue;

		switch (Functor(Item.ExtensionStruct))
		{
		case Utils::Break:
			// Exit loop
			return;
		case Utils::Continue:
			break;
		}
	}
}

void FFaerieItemContainerExtensions::ForEach_Recursive(
	const TFunctionRef<Utils::EIteratorFunctorReturn(FConstStructView)>& Functor) const
{
	for (const FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (!Item.ExtensionStruct.IsValid()) continue;

		switch (Functor(Item.ExtensionStruct))
		{
		case Utils::Break:
			// Exit loop
			return;
		case Utils::Continue:
			break;
		}
	}

	if (ParentExtensions.Key.IsValid())
	{
		ParentExtensions.Value->ForEach_Recursive(Functor);
	}
}

void FFaerieItemContainerExtensions::ForEachMutable_Recursive(
	const TFunctionRef<Utils::EIteratorFunctorReturn(FStructView)>& Functor)
{
	for (FFaerieItemContainerExtensionStorageElement& Item : Items)
	{
		if (!Item.ExtensionStruct.IsValid()) continue;

		switch (Functor(Item.ExtensionStruct))
		{
		case Utils::Break:
			// Exit loop
			return;
		case Utils::Continue:
			break;
		}
	}

	if (ParentExtensions.Key.IsValid())
	{
		ParentExtensions.Value->ForEachMutable_Recursive(Functor);
	}
}

UFaerieItemContainerBase::UFaerieItemContainerBase()
{
}

void UFaerieItemContainerBase::PostInitProperties()
{
	Super::PostInitProperties();

	// Bind replication functions out into this class.
	ExtensionData.ChangeListener = this;
}

void UFaerieItemContainerBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ExtensionData)
}

void UFaerieItemContainerBase::InitializeNetObject(const TNotNull<AActor*> Actor)
{
	ensureAlwaysMsgf(!Faerie::Utils::HasLoadFlag(this),
		TEXT("Containers must not be assets loaded from disk. (DuplicateObjectFromDiskForReplication or ClearLoadFlags can fix this)"
			LINE_TERMINATOR
			"	Failing Container: '%s'"), *GetFullName());
	InitializeExtensions();
}

void UFaerieItemContainerBase::OnItemDataChanged(const FFaerieItemInstance& Instance, const FGameplayTag EditTag)
{
}

FFaerieItemExportData UFaerieItemContainerBase::ExportItemData(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item) const
{
	static constexpr ItemData::EMassFragmentExportOptions ExportOptions = ItemData::EMassFragmentExportOptions::OnlyFaerieMassFragments;

	FFaerieItemExportData ExportData;
	Item.ExportFragmentData(EntityManager, ExportData.MassInstances, ExportOptions);
	return ExportData;
}

FFaerieItemInstance UFaerieItemContainerBase::ImportItemData(FMassEntityManager& EntityManager, const UFaerieItem* Item,
	const FFaerieItemExportData& ExportData)
{
	FFaerieItemInstance Instance = FFaerieItemInstance::FromPointer(Item);

	// Make a copy of the instance data, as the elements are moved into place.
	TArray<FInstancedStruct> FragmentCopy = ExportData.MassInstances;
	Instance.ImportFragmentData(EntityManager, FragmentCopy);
	return Instance;
}

void UFaerieItemContainerBase::RavelExtensionData(TAdderRef<FInstancedStruct> SaveData) const
{
	ExtensionData.ForEach([this, &SaveData](const FConstStructView Element)
		{
			SaveData.Add(FInstancedStruct(Element));
			return Utils::Continue;
		});
}

void UFaerieItemContainerBase::UnravelExtensionData(const TConstArrayView<FInstancedStruct> SaveData)
{
	for (auto&& Element : SaveData)
	{
		FInstancedStruct& Extension = ExtensionData.AddOrGetRef(Element.GetScriptStruct());
		Extension.InitializeAs(Element.GetScriptStruct(), Element.GetMemory());
	}
}

// Note: Implementations for these PURE_VIRTUAL need to be here because TUniquePtr complains about their dtors if they are forward declared.
TUniquePtr<Container::IEntryIterator> UFaerieItemContainerBase::CreateEntryIterator() const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateEntryIterator, return nullptr; )

TUniquePtr<Container::IAddressIterator> UFaerieItemContainerBase::CreateAddressIterator() const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateAddressIterator, return nullptr; )

TUniquePtr<Container::IAddressIterator> UFaerieItemContainerBase::CreateSingleEntryIterator(FFaerieEntryKey Key) const
	PURE_VIRTUAL(UFaerieItemContainerBase::CreateSingleEntryIterator, return nullptr; )

void UFaerieItemContainerBase::WriteContainerData(const TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView, FFaerieItemContainerExtensions::EWriteContainerDataFlag)>& Functor, const bool CreateIfMissing)
{
	FFaerieItemContainerExtensions::EWriteContainerDataFlag DataFlag;
	if (CreateIfMissing)
	{
		auto& Element = ExtensionData.AddOrGetRef(Type, &DataFlag);
		Functor(Element, DataFlag);
	}
	else
	{
		if (const FStructView ElementView = ExtensionData.Find(Type, false);
			ElementView.IsValid())
		{
			Functor(ElementView, FFaerieItemContainerExtensions::EWriteContainerDataFlag::None);
		}
	}
}

void UFaerieItemContainerBase::WriteContainerData(const TNotNull<const UScriptStruct*> Type, const TFunctionRef<void(FStructView)>& Functor, const bool CreateIfMissing)
{
	FFaerieItemContainerExtensions::EWriteContainerDataFlag DataFlag;
	if (CreateIfMissing)
	{
		auto& Element = ExtensionData.AddOrGetRef(Type, &DataFlag);
		Functor(Element);
	}
	else
	{
		if (const FStructView ElementView = ExtensionData.Find(Type, false);
			ElementView.IsValid())
		{
			Functor(ElementView);
		}
	}
}

bool UFaerieItemContainerBase::HasContainerData(const TNotNull<const UScriptStruct*> Type, const bool RecurseParents) const
{
	if (const FConstStructView Value = ExtensionData.Find(Type, RecurseParents);
		Value.IsValid())
	{
		return true;
	}

	return false;
}

FConstStructView UFaerieItemContainerBase::ReadContainerData(const TNotNull<const UScriptStruct*> Type, const bool RecurseParents) const
{
	if (const FConstStructView Value = ExtensionData.Find(Type, RecurseParents);
		Value.IsValid())
	{
		return Value;
	}

	return FConstStructView();
}

void UFaerieItemContainerBase::SetParentExtensions(TNotNull<UObject*> Obj, FFaerieItemContainerExtensions& InExtensions)
{
	ExtensionData.ParentExtensions = MakeTuple(Obj, &InExtensions);
}

void UFaerieItemContainerBase::ClearParentExtensions()
{
	ExtensionData.ParentExtensions = MakeTuple(nullptr, nullptr);
}

void UFaerieItemContainerBase::InitializeExtensions()
{
	// Note: Not recursive, we only init *our* own extensions, not our parents.
	ExtensionData.ForEachMutable([this](const FStructView Element)
		{
			if (FFaerieItemContainerExtensionBase* AsExtensionBase = Element.GetPtr<FFaerieItemContainerExtensionBase>())
			{
				AsExtensionBase->InitializeExtension(this);
			}
			return Utils::Continue;
		});
}

bool UFaerieItemContainerBase::AllowsAddition(const Utils::TArrayAdapter<FFaerieItemProxy>& Proxies,
	const FFaerieExtensionAllowsAdditionArgs Args, const bool DefaultResult) const
{
	TOptional<bool> AllowedByStructs;
	ExtensionData.ForEach_Recursive([this, Proxies, Args, &AllowedByStructs](const FConstStructView Element)
		{
			if (const FFaerieItemContainerExtensionBase* AsExtensionBase = Element.GetPtr<FFaerieItemContainerExtensionBase>())
			{
				switch (AsExtensionBase->AllowsAddition(this, Proxies, Args))
				{
				case EFaerieExtensionResponse::Disallowed:
					// Mark disallowal, and exit loop
					AllowedByStructs = false;
					return Utils::Break;
				case EFaerieExtensionResponse::Allowed:
					AllowedByStructs = true;
					break;
				case EFaerieExtensionResponse::NoExplicitResponse:
					default:
					break;
				}
			}
			return Utils::Continue;
		});

	if (AllowedByStructs.IsSet())
	{
		return AllowedByStructs.GetValue();
	}
	return DefaultResult;
}

bool UFaerieItemContainerBase::AllowsRemoval(const TNotNull<const Container::IAddressView*> DataView,
	const FFaerieInventoryTag Reason, const bool DefaultResult) const
{
	TOptional<bool> AllowedByStructs;

	// Check this container for permission
	if (auto ContainerPermissions = ReadContainerData<FFaerieItemContainerClientPermissions>(true))
	{
		if (ContainerPermissions->AllowedActions.HasTag(Reason))
		{
			AllowedByStructs = true;
		}
	}

	// Check the item instance for permission override.
	auto EntityManager = ItemData::GetFaerieEntityManager();
	if (const TConstStructView<FFaerieContainerMetadataFragment> Metadata = Faerie::ItemData::GetEntityFragmentOrDefault<FFaerieContainerMetadataFragment>(EntityManager, DataView->GetItemInstance().GetValue());
		Metadata.IsValid())
	{
		if (Metadata->AllowedActions.HasTag(Reason))
		{
			AllowedByStructs = true;
		}
		if (Metadata->DisallowedActions.HasTag(Reason))
		{
			AllowedByStructs = false;
		}
	}

	ExtensionData.ForEach_Recursive([this, DataView, Reason, &AllowedByStructs](const FConstStructView Element)
		{
			if (const FFaerieItemContainerExtensionBase* AsExtensionBase = Element.GetPtr<FFaerieItemContainerExtensionBase>())
			{
				switch (AsExtensionBase->AllowsRemoval(this, DataView, Reason))
				{
				case EFaerieExtensionResponse::Disallowed:
					// Mark disallowal, and exit loop
					AllowedByStructs = false;
					return Utils::Break;
				case EFaerieExtensionResponse::Allowed:
					AllowedByStructs = true;
					break;
				case EFaerieExtensionResponse::NoExplicitResponse:
				default:
					break;
				}
			}
			return Utils::Continue;
		});

	if (AllowedByStructs.IsSet())
	{
		return AllowedByStructs.GetValue();
	}
	return DefaultResult;
}

bool UFaerieItemContainerBase::AllowsEdit(const TNotNull<const Container::IAddressView*> DataView,
	const FFaerieInventoryTag EditTag, const bool DefaultResult) const
{
	TOptional<bool> AllowedByStructs;
	ExtensionData.ForEach_Recursive([this, DataView, EditTag, &AllowedByStructs](const FConstStructView Element)
		{
			if (const FFaerieItemContainerExtensionBase* AsExtensionBase = Element.GetPtr<FFaerieItemContainerExtensionBase>())
			{
				switch (AsExtensionBase->AllowsEdit(this, DataView, EditTag))
				{
				case EFaerieExtensionResponse::Disallowed:
					// Mark disallowal, and exit loop
					AllowedByStructs = false;
					return Utils::Break;
				case EFaerieExtensionResponse::Allowed:
					AllowedByStructs = true;
					break;
				case EFaerieExtensionResponse::NoExplicitResponse:
				default:
					break;
				}
			}
			return Utils::Continue;
		});

	if (AllowedByStructs.IsSet())
	{
		return AllowedByStructs.GetValue();
	}
	return DefaultResult;
}

void UFaerieItemContainerBase::PostEvent(const Inventory::FEventData& Event, const FFaerieInventoryTag Reason)
{
	const Inventory::FEventLogBatch Batch(MakeConstArrayView(&Event, 1), Reason);

	FInstancedStruct EventStruct;
	EventStruct.InitializeAs<Container::FEvent>(Container::FEvent{
		.Timestamp = Batch.GetTimestamp(),
		.Container = this,
		.Instance = Event.Instance,
		.Type = Reason,
		.Copies = Event.Copies,
		.EntryTouched = Event.EntryTouched,
		.AddressesTouched = Event.AddressesTouched,
		.EntryRemoved = Event.EntryRemoved,
	});

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	if (const Container::FNestedContainer* Nesting = ReadContainerData<Container::FNestedContainer>())
	{
		// @todo we don't pass in the ItemAsset here, which isn't correct, but also isn't likely to matter
		FFaerieItemInstance Instance(nullptr, Nesting->ItemHandle);
		Instance.TempNestedContainerChanged(EntityManager);
	}

	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[EventStruct](FMassEntityManager& DeferredEntityManager)
		{
			DeferredEntityManager.CreateEntity(MakeConstArrayView(&EventStruct, 1));
		});
}

void UFaerieItemContainerBase::PostEventBatch(const Inventory::FEventLogBatch& Events)
{
	for (const Inventory::FEventData& Event : Events.Data)
	{
		FInstancedStruct EventStruct;
		EventStruct.InitializeAs<Container::FEvent>(Container::FEvent{
			.Timestamp = Events.GetTimestamp(),
			.Container = this,
			.Instance = Event.Instance,
			.Type = Events.Type,
			.Copies = Event.Copies,
			.EntryTouched = Event.EntryTouched,
			.AddressesTouched = Event.AddressesTouched,
			.EntryRemoved = Event.EntryRemoved,
		});

		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

		EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
			[EventStruct](FMassEntityManager& DeferredEntityManager)
			{
				DeferredEntityManager.CreateEntity(MakeConstArrayView(&EventStruct, 1));
			});
	}
}