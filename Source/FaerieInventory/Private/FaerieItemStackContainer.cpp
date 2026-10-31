// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStackContainer.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemDestructionProcessor.h"
#include "FaerieItemOwnership.h"
#include "FaerieStorageEnums.h"
#include "FaerieUnownedItemStack.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"

#include "Fragments/FaerieItemStorageFragment.h"

#include "MassReplication/FaerieMassReplicationSubsystem.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStackContainer)

using namespace Faerie;

namespace Faerie::Container
{
	namespace
	{
		/**
		 * Not really an iterator, this just exists to implement the Container::IEntryIterator, so we can transparently interop
		 * with generic Container API that consumes iterators.
		 */
		class FEntryContainerIteratorStub final : public IEntryIterator
		{
		public:
			UE_REWRITE explicit FEntryContainerIteratorStub(const UFaerieItemStackContainer* Stack)
			  : Stack(Stack) {}

			//~ ItemData::IViewBase
			UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Stack->GetCurrentKey(); }
			UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return Stack->GetEntityHandle(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			//~ ItemData::IViewBase

			//~ Container::IEntryIterator
			UE_REWRITE virtual void Advance() override { Stack = nullptr; }
			UE_REWRITE virtual bool IsValid() const override { return ::IsValid(Stack) && Stack->IsFilled(); }
			//~ Container::IEntryIterator

		private:
			const UFaerieItemStackContainer* Stack;
		};

		/**
		 * Not really an iterator, this just exists to implement the Container::IAddressIterator, so we can transparently interop
		 * with generic Container API that consumes iterators or views.
		 */
		class FStackContainerIteratorStub final : public IAddressIterator
		{
		public:
			UE_REWRITE explicit FStackContainerIteratorStub(const UFaerieItemStackContainer* Stack)
			  : Stack(Stack) {}

			//~ ItemData::IViewBase
			UE_REWRITE virtual FFaerieEntryKey ResolveKey() const override { return Stack->GetCurrentKey(); }
			UE_REWRITE virtual FFaerieAddress ResolveAddress() const override { return Stack->GetCurrentAddress(); }
			UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return Stack->GetEntityHandle(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			//~ ItemData::IViewBase

			//~ Container::IAddressIterator
			UE_REWRITE virtual void Advance() override { Stack = nullptr; }
			UE_REWRITE virtual bool IsValid() const override { return ::IsValid(Stack) && Stack->IsFilled(); }
			//~ Container::IAddressIterator

		private:
			const UFaerieItemStackContainer* Stack;
		};
	}
}

const UScriptStruct* FFaerieSimpleItemStackSaveData::GetFragmentType() const
{
	return FFaerieChildStackFragment::StaticStruct();
}

void UFaerieItemStackContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// State members are push based
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemStack, SharedParams)
}

FInstancedStruct UFaerieItemStackContainer::MakeSaveData(const FMassEntityManager& EntityManager, const Container::FSaveParams Params) const
{
	FFaerieSimpleItemStackSaveData SlotSaveData;
	MakeSaveData(EntityManager, SlotSaveData, Params);
	return FInstancedStruct::Make(SlotSaveData);
}

bool UFaerieItemStackContainer::LoadSaveData(FMassEntityManager& EntityManager, const FConstStructView ItemData, const Container::FLoadParams Params)
{
	const FFaerieSimpleItemStackSaveData* SaveData = ItemData.GetPtr<const FFaerieSimpleItemStackSaveData>();
	if (!SaveData)
	{
		UE_LOGF(LogFaerieInventory, Error, "Invalid data to load from. Must be FFaerieSimpleItemStackSaveData.")
		return false;
	}

	LoadSaveData(EntityManager, *SaveData, Params);
	return true;
}

bool UFaerieItemStackContainer::Contains(const FFaerieAddress Address) const
{
	return IsOurAddress(Address);
}

FMassEntityHandle UFaerieItemStackContainer::GetItemHandle(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return ItemEntity;
	}
	return FMassEntityHandle();
}

FMassEntityHandle UFaerieItemStackContainer::GetItemHandle(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return ItemEntity;
	}
	return FMassEntityHandle();
}

ItemData::FScopeProxy UFaerieItemStackContainer::ViewEntry(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return GetView();
	}
	return nullptr;
}

ItemData::FScopeProxy UFaerieItemStackContainer::ViewAddress(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return GetView();
	}
	return nullptr;
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FMassEntityManager& EntityManager, const FMassEntityHandle Item) const
{
	if (IsOurItem(Item))
	{
		return FFaerieItemProxy(EntityManager.AsShared(), this);
	}
	return FFaerieItemProxy();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FMassEntityManager& EntityManager, const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return FFaerieItemProxy(EntityManager.AsShared(), this);
	}
	return FFaerieItemProxy();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FMassEntityManager& EntityManager, const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return FFaerieItemProxy(EntityManager.AsShared(), this);
	}
	return FFaerieItemProxy();
}

bool UFaerieItemStackContainer::Possess(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack)
{
	return SetItemInSlot(EntityManager, Stack);
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(FMassEntityManager& EntityManager, const FFaerieEntryKey Key, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (IsOurKey(Key))
	{
		return TakeItemFromSlot(EntityManager, Copies, Reason);
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(FMassEntityManager& EntityManager, const FFaerieAddress Address, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (IsOurAddress(Address))
	{
		return TakeItemFromSlot(EntityManager, Copies, Reason);
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (Proxy.GetProxyObject() == this)
	{
		return TakeItemFromSlot(EntityManager, Copies, Reason);
	}
	return NullOpt;
}

bool UFaerieItemStackContainer::CanPossess(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy) const
{
	return CanSetInSlot(EntityManager, Proxy);
}

bool UFaerieItemStackContainer::CanRelease(const FMassEntityManager& EntityManager, const FFaerieItemProxy& Proxy, const FFaerieInventoryTag Reason) const
{
	if (Proxy.GetProxyObject() == this)
	{
		return CanTakeFromSlot(EntityManager, Proxy.GetCopies(), Reason);
	}
	return false;
}

void UFaerieItemStackContainer::GetAllAddresses(const TAdderReserverRef<FFaerieAddress> Addresses) const
{
	if (IsFilled())
	{
		Addresses.Add(GetCurrentAddress());
	}
}

bool UFaerieItemStackContainer::AckItemCreatedByNetwork(FMassEntityManager& EntityManager, const FMassEntityHandle Entity, const FFaerieNetItemID NetID)
{
	if (!NetID.IsSet())
	{
		UE_LOGF(LogFaerieInventory, Error, "Ack failed: invalid NetID")
		return false;
	}

	if (NetID == ItemStack.NetID && Entity == ItemEntity)
	{
		// We already have this NetID/Entity pair... tell the replication subsystem all is fine.
		return true;
	}

	if (ItemStack.NetID != NetID)
	{
		// We cannot accept a new entity from the server, until we have got our own ItemStack replication up to date.
		// This is to prevent broadcasting an event, while the StoredKey is invalid, or copies is 0.
		UE_LOGF(LogFaerieInventory, Warning, "Ack failed: mismatch from current NetID. Could be waiting for replication to catch up.")
		return false;
	}

	ItemEntity = Entity;

	const FFaerieAddress Address = GetCurrentAddress();
	const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this,
		ItemEntity, ItemStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
	PostEvent(EntityManager, Event);

	BroadcastChange(EntityManager, Inventory::Tags::NewStack);

	return true;
}

void UFaerieItemStackContainer::OnItemDataChanged(FMassEntityManager& EntityManager, const FMassEntityHandle Item, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(EntityManager, Item, EditTag);
	check(ItemEntity == Item);

	BroadcastChange(EntityManager, Inventory::Tags::ReplicationEdit);
}

TUniquePtr<Container::IEntryIterator> UFaerieItemStackContainer::CreateEntryIterator() const
{
	// Don't provide an iterator if we are empty...
	if (!IsFilled()) return nullptr;
	return MakeUnique<Container::FEntryContainerIteratorStub>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStackContainer::CreateAddressIterator() const
{
	// Don't provide an iterator if we are empty...
	if (!IsFilled()) return nullptr;
	return MakeUnique<Container::FStackContainerIteratorStub>(this);
}

TUniquePtr<Container::IAddressIterator> UFaerieItemStackContainer::CreateSingleEntryIterator(const FFaerieEntryKey Key) const
{
	if (GetCurrentKey() != Key) return nullptr;
	return MakeUnique<Container::FStackContainerIteratorStub>(this);
}

FMassEntityHandle UFaerieItemStackContainer::GetEntityHandle() const
{
	return ItemEntity;
}

int32 UFaerieItemStackContainer::GetCopies() const
{
	return ItemStack.Copies;
}

FFaerieAddress UFaerieItemStackContainer::Proxy_GetAddress() const
{
	return GetCurrentAddress();
}

FFaerieItemNetworkHandle UFaerieItemStackContainer::Proxy_GetNetworkHandle() const
{
	return GetNetworkHandle();
}

void UFaerieItemStackContainer::MakeSaveData(const FMassEntityManager& EntityManager, FFaerieSimpleItemStackSaveData& SaveData, const Container::FSaveParams Params) const
{
	if (Params.ExportItemData)
	{
		if (ItemStack.StoredKey.IsValid())
        {
        	SaveData.ExportData = ExportItemData(EntityManager, ItemEntity);
			SaveData.Copies = ItemStack.Copies;
        }
	}

	if (Params.ExportExtensionData)
	{
		RavelExtensionData(EntityManager, SaveData.ExtensionData);
	}
}

void UFaerieItemStackContainer::LoadSaveData(FMassEntityManager& EntityManager, const FFaerieSimpleItemStackSaveData& SaveData, const Container::FLoadParams Params)
{
	// Clear out state
	if (Params.ClearExtensionsBeforeImport)
	{
		ExtensionData.Reset();
	}

	if (Params.ClearItemDataBeforeImport)
	{
		// Clear any current content.
		if (IsFilled())
		{
			TakeItemFromSlot(EntityManager, ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
		}
	}

	if (SaveData.Copies > 0)
	{
		// Rebuild item from save data
		FMassEntityHandle Item;
		{
			Container::FBlockTakeOwnershipScope BlockTakeOwnershipDuringImport;
			Item = ImportItemData(EntityManager, SaveData.ExportData);
		}

		if (Item.IsSet() && SaveData.Copies > 0)
		{
			// If it validated, store in slot.
			const FFaerieUnownedItemStack Stack(Item, SaveData.Copies);
			SetStoredItem_Impl(EntityManager, Stack);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOGF(LogFaerieInventory, Error, "Loading content for stack container '%ls' failed. Stored item has been reset!", *GetPathName())
			MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieItemStackContainer, ItemStack, this);
			ItemStack.StoredKey = FFaerieEntryKey::InvalidKey;
		}
	}

	UnravelExtensionData(EntityManager, SaveData.ExtensionData);
	InitializeExtensions(EntityManager);
}

FFaerieAddress UFaerieItemStackContainer::GetAddress() const
{
	return GetCurrentAddress();
}

void UFaerieItemStackContainer::BroadcastChange(FMassEntityManager& EntityManager, const FFaerieInventoryTag Event)
{
	OnItemChangedNative.Broadcast(FFaerieItemProxy(EntityManager.AsShared(), this), Event);
	OnItemChanged.Broadcast(this, Event);
}

bool UFaerieItemStackContainer::IsOurItem(const FMassEntityHandle Item) const
{
	return ItemEntity == Item;
}

bool UFaerieItemStackContainer::IsOurKey(const FFaerieEntryKey Key) const
{
	return ItemStack.StoredKey == Key;
}

bool UFaerieItemStackContainer::IsOurAddress(const FFaerieAddress Address) const
{
	return static_cast<int32>(Address.Address) == ItemStack.StoredKey.Value();
}

void UFaerieItemStackContainer::SetStoredItem_Impl(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& ValidStack)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (ValidStack.Item != ItemEntity)
	{
		// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
		ItemStack.StoredKey = KeyGen.NextKey();
		ItemStack.Copies = ValidStack.Copies;
		ItemEntity = ValidStack.Item;

		// Take ownership of the new item if it's mutable
		if (ItemData::IsMutable(EntityManager, ItemEntity))
		{
			Container::TakeOwnership(EntityManager, this, ItemEntity);
		}

		// @Todo add way to disable these this

		// Find or create NetId for item.
		if (const FFaerieNetIDFragment* ExistingNetID = EntityManager.GetFragmentDataPtr<FFaerieNetIDFragment>(ItemEntity))
		{
			ItemStack.NetID = ExistingNetID->NetID;
		}
		else
		{
			// @Todo better netId generation
			ItemStack.NetID = { static_cast<uint64>(static_cast<double>(FMath::FRand()) * TNumericLimits<uint64>::Max()) };
			FInstancedStruct NetID;
			NetID.InitializeAs<FFaerieNetIDFragment>(FFaerieNetIDFragment{.NetID = ItemStack.NetID});
			EntityManager.AddFragmentInstanceListToEntity(ItemEntity, MakeConstArrayView(&NetID, 1));
			EntityManager.AddSparseElementToEntity<Container::FNeedsReplicationInit>(ItemEntity);
		}

		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this,
			ItemEntity, ValidStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));

		PostEvent(EntityManager, Event);

		BroadcastChange(EntityManager, Inventory::Tags::NewStack);
	}
	else
	{
		ItemStack.Copies += ValidStack.Copies;

		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeAddedCopies(this,
			ItemEntity, ValidStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));

		PostEvent(EntityManager, Event);

		BroadcastChange(EntityManager, Inventory::Tags::AddedCopies);
	}
}

#if WITH_EDITOR
void UFaerieItemStackContainer::Editor_ClearData()
{
	ItemStack = FFaerieStackContainerContent();
	ItemEntity.Reset();
}
#endif

ItemData::FScopeProxy UFaerieItemStackContainer::GetView() const
{
	return ItemData::FScopeProxy(ItemEntity, ItemStack.Copies);
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.HasValidStack()) return false;

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = {
		.CheckType = EFaerieContainerAddStackCheckType::CouldEverAdd,
		.AddStackBehavior = EFaerieStorageAddStackBehavior::OnlyNewStacks
	};

	return AllowsAddition(EntityManager, MakeConstArrayView(&Proxy, 1), Args, true);
}

bool UFaerieItemStackContainer::CanSetInSlot(const FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.HasValidStack()) return false;

	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (Proxy.GetEntityHandle() != ItemEntity)
		{
			return false;
		}
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = {
		.CheckType = EFaerieContainerAddStackCheckType::CanAddNow,
		.AddStackBehavior = EFaerieStorageAddStackBehavior::OnlyNewStacks
	};
	return AllowsAddition(EntityManager, MakeConstArrayView(&Proxy, 1), Args, true);
}

bool UFaerieItemStackContainer::CanTakeFromSlot(const FMassEntityManager& EntityManager, const int32 Copies,
	const FFaerieInventoryTag Reason) const
{
	if (!IsFilled()) return false;

	if (Copies != ItemData::EntireStack &&
		ItemStack.Copies < Copies)
	{
		return false;
	}

	Container::FStackContainerIteratorStub ViewStub(this);
	return AllowsRemoval(EntityManager, &ViewStub, Reason, true);
}

bool UFaerieItemStackContainer::SetItemInSlot(FMassEntityManager& EntityManager,
	const FFaerieUnownedItemStack& Stack)
{
	if (!Stack.IsValid()) return false;

	const ItemData::FScopeProxy StackProxy(Stack.Item, Stack.Copies);
	if (!CanSetInSlot(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManager.AsShared(), &StackProxy)))
	{
		return false;
	}

	// If the above check passes, then either the Stack's item is the same as ours, or we are currently empty!
	SetStoredItem_Impl(EntityManager, Stack);
	return true;
}

FFaerieUnownedItemStack UFaerieItemStackContainer::TakeItemFromSlot(FMassEntityManager& EntityManager, int32 Copies, const FFaerieInventoryTag Reason)
{
	if (!CanTakeFromSlot(EntityManager, Copies, Reason))
	{
		UE_LOGF(LogFaerieInventory, Warning,
			"Invalid request to take item from container '%ls'!", *GetPathName())
		return FFaerieUnownedItemStack();
	}

	if (Copies > ItemStack.Copies)
	{
		UE_LOGF(LogFaerieInventory, Error,
			"Cannot remove more copies from a container than what it contains. Container: '%ls', Requested Copies: '%i', Contained: '%i' !",
			*GetPathName(), Copies, ItemStack.Copies)
		return FFaerieUnownedItemStack();
	}

	if (Copies == ItemData::EntireStack)
	{
		Copies = ItemStack.Copies;
	}

	const FFaerieAddress CurrentAddress = GetCurrentAddress();
	const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this,
		ItemEntity, Copies, Reason, ItemStack.StoredKey,
		MakeConstArrayView(&CurrentAddress, 1), Copies == ItemStack.Copies);

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Event.EntryRemoved)
	{
		ItemStack.StoredKey = FFaerieEntryKey::InvalidKey;

		// Our local Item ptr must be nullptr before calling ReleaseOwnership so that extensions don't see that we have
		// this item. Extension logic is triggered by ReleaseOwnership as it calls ClearParentGroup on any subobjects.
		ItemStack = FFaerieStackContainerContent();

		// Release ownership of this item.
		if (ItemData::IsMutable(EntityManager, Event.Item))
		{
			Container::ReleaseOwnership(EntityManager, this, Event.Item);
		}
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	PostEvent(EntityManager, Event);

	BroadcastChange(EntityManager, Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		ItemData::MarkItemAsGarbage(EntityManager, Event.Item);
		return FFaerieUnownedItemStack();
	}

	return FFaerieUnownedItemStack(Event.Item, Event.Copies);
}

int32 UFaerieItemStackContainer::GetStackCopies() const
{
	return ItemStack.Copies;
}

FFaerieEntryKey UFaerieItemStackContainer::GetCurrentKey() const
{
	return ItemStack.StoredKey;
}

FFaerieAddress UFaerieItemStackContainer::GetCurrentAddress() const
{
	return FFaerieAddress(ItemStack.StoredKey.Value());
}

FFaerieItemNetworkHandle UFaerieItemStackContainer::GetNetworkHandle() const
{
	return FFaerieItemNetworkHandle {.Container = const_cast<ThisClass*>(this), .Address = GetCurrentAddress()};
}

bool UFaerieItemStackContainer::IsFilled() const
{
	// Note: we don't check if ItemEntity is valid/set, because we might be a client that has had an item "set" on us
	// by the server, but not yet ack'd, in which case only our NetID is valid.
	return ItemStack.NetID.IsSet() && ItemStack.Copies > 0;
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FFaerieItemProxy& Proxy) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return CouldSetInSlot(EntityManager, Proxy);
}

bool UFaerieItemStackContainer::CanSetInSlot(const FFaerieItemProxy& Proxy) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return CanSetInSlot(EntityManager, Proxy);
}

bool UFaerieItemStackContainer::CanTakeFromSlot(const int32 Copies, const FFaerieInventoryTag Reason) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return CanTakeFromSlot(EntityManager, Copies, Reason);
}

bool UFaerieItemStackContainer::SetItemInSlot(const FFaerieUnownedItemStack& Stack)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return SetItemInSlot(EntityManager, Stack);
}

FFaerieUnownedItemStack UFaerieItemStackContainer::TakeItemFromSlot(const int32 Copies, const FFaerieInventoryTag Reason)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return TakeItemFromSlot(EntityManager, Copies, Reason);
}

void UFaerieItemStackContainer::OnRep_ItemStack(const FFaerieStackContainerContent& OldValue)
{
	UE_LOGF(LogFaerieInventory, Log, "UFaerieItemStackContainer::OnRep_ItemStack for %ls.%ls. OldValue: (Key: %ls, ID: %llu, Copies: %i). NewValue (Key: %ls, ID: %llu, Copies: %i).",
		*GetNameSafe(GetTypedOuter<AActor>()), *GetNameSafe(this),
		*OldValue.StoredKey.ToString(), OldValue.NetID.ID, OldValue.Copies,
		*ItemStack.StoredKey.ToString(), ItemStack.NetID.ID, ItemStack.Copies)

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	// Case 1: Item changed by server.
	if (ItemStack.NetID != OldValue.NetID)
	{
		if (ItemStack.Copies == 0)
		{
			if (ensure(!ItemStack.NetID.IsSet()))
			{
				UE_LOGF(LogFaerieInventory, Error, "Server replicated item stack with invalid NetID. This stack will accept next ack'd item, but this should be avoided!")
				ItemStack = FFaerieStackContainerContent();
				ItemEntity.Reset();
				return;
			}

			/*
			 * Note: the client doesn't know if the items were moved or deleted, so better to treat them as deleted downstream.
			 * From the clients perspective they may as well have been deleted, because the server may have moved them out of
			 * the sight of the client.
			 */
			const int32 RemovedCopies = OldValue.Copies;
			const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this,
				ItemEntity, RemovedCopies, Inventory::Tags::RemovalDeletion, OldValue.StoredKey, {}, true);
			PostEvent(EntityManager, Event);
			BroadcastChange(EntityManager, Inventory::Tags::RemovalDeletion);

			// Clear local item entity.
			ItemEntity.Reset();
		}
		else
		{
			if (!ensure(ItemStack.Copies > 0))
			{
				UE_LOGF(LogFaerieInventory, Error, "Server replicated item stack with empty stack! This should not happen!")
			}
			ensure(ItemStack.NetID.IsSet());
			ensure(ItemStack.StoredKey.IsValid());

			UFaerieMassReplicationSubsystem* Replication = GetWorld()->GetSubsystem<UFaerieMassReplicationSubsystem>();
			if (TOptional<FMassEntityHandle> Item = Replication->ResolveNetId(ItemStack.NetID);
				Item.IsSet())
			{
				ItemEntity = Item.GetValue();

				const FFaerieAddress Address = GetCurrentAddress();
				const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this,
					ItemEntity, ItemStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
				PostEvent(EntityManager, Event);

				BroadcastChange(EntityManager, Inventory::Tags::NewStack);
			}
			else
			{
				// Clear local item entity. New item is not valid, until Ack'd to us by the replicator.
				ItemEntity.Reset();
			}
		}
	}
	// Case 2: Same item, added to stack.
	else if (ItemStack.Copies > OldValue.Copies)
	{
		const int32 AddedCopies = ItemStack.Copies - OldValue.Copies;
		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeAddedCopies(this,
			ItemEntity, AddedCopies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
		PostEvent(EntityManager, Event);

		BroadcastChange(EntityManager, Inventory::Tags::AddedCopies);
	}
	// Case 3: Same item, removed from stack.
	else if (ItemStack.Copies < OldValue.Copies)
	{
		/*
		 * Note: the client doesn't know if the items were moved or deleted, so better to treat them as deleted downstream.
		 * From the clients perspective they may as well have been deleted, because the server may have moved them out of
		 * the sight of the client.
		 */
		const int32 RemovedCopies = OldValue.Copies - ItemStack.Copies;
		const FFaerieAddress Address(ItemStack.StoredKey.Value());
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this,
			ItemEntity, RemovedCopies, Inventory::Tags::RemovalDeletion, OldValue.StoredKey,
			MakeConstArrayView(&Address, 1), !IsFilled());
		PostEvent(EntityManager, Event);
		BroadcastChange(EntityManager, Inventory::Tags::RemovedCopies);
	}
	else
	{
		// Nothing is changed? It's possible that it was actually the NetID that was changed, but before this OnRep occured,
		// we were already ack'd by the replication subsystem and already received the new NetID.
		UE_LOGF(LogFaerieInventory, Warning, "Client received OnRep, but doesn't know what changed...")
	}
}