// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStackContainer.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerEvent.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
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
			UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Stack->GetItemInstance(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			UE_REWRITE virtual const UObject* GetItemOwner() const override { return Stack; }
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
			UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Stack->GetItemInstance(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			UE_REWRITE virtual const UObject* GetItemOwner() const override { return Stack; }
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
		return ItemStack.Instance.GetMassEntityHandle();
	}
	return FMassEntityHandle();
}

FMassEntityHandle UFaerieItemStackContainer::GetItemHandle(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return ItemStack.Instance.GetMassEntityHandle();
	}
	return FMassEntityHandle();
}

TOptional<FFaerieItemInstance> UFaerieItemStackContainer::ViewInstance(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return ItemStack.Instance;
	}
	return NullOpt;
}

TOptional<FFaerieItemInstance> UFaerieItemStackContainer::ViewInstance(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return ItemStack.Instance;
	}
	return NullOpt;
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

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FMassEntityHandle Item) const
{
	if (IsOurItem(Item))
	{
		return FFaerieItemProxy(this);
	}
	return FFaerieItemProxy();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return FFaerieItemProxy(this);
	}
	return FFaerieItemProxy();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return FFaerieItemProxy(this);
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
	if (NetID == ItemStack.NetID)
	{
		ItemStack.Instance = FFaerieItemInstance::FromEntity(Entity);

		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this, ItemStack.Instance, ItemStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
		PostEvent(EntityManager, Event);

		BroadcastChange(Inventory::Tags::NewStack);

		return true;
	}
	return false;
}

void UFaerieItemStackContainer::OnItemDataChanged(const FMassEntityHandle Item, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(Item, EditTag);
	check(ItemStack.Instance.GetMassEntityHandle() == Item);

	BroadcastChange(Inventory::Tags::ReplicationEdit);
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
	if (!ItemStack.Instance.IsEmpty())
	{
		return ItemStack.Instance.GetMassEntityHandle();
	}
	return FMassEntityHandle();
}

TOptional<FFaerieItemInstance> UFaerieItemStackContainer::GetItemInstance() const
{
	if (!ItemStack.Instance.IsEmpty())
	{
		return ItemStack.Instance;
	}
	return NullOpt;
}

int32 UFaerieItemStackContainer::GetCopies() const
{
	return ItemStack.Copies;
}

const UObject* UFaerieItemStackContainer::GetItemOwner() const
{
	return this;
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
        	SaveData.ItemObject = ItemStack.Instance.GetItemPtr();
        	SaveData.Copies = ItemStack.Copies;
        	SaveData.ExportData = ExportItemData(EntityManager, ItemStack.Instance.GetMassEntityHandle());
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
		// Rebuild instance from save data
		FFaerieItemInstance Instance;
		{
			Container::FBlockTakeOwnershipScope BlockTakeOwnershipDuringImport;
			Instance = ImportItemData(EntityManager, SaveData.ItemObject, SaveData.ExportData);
		}

		if (Container::ValidateItemData(Instance) &&
			SaveData.Copies > 0)
		{
			// If it validated, store in slot.
			const FFaerieUnownedItemStack Stack(Instance, SaveData.Copies);
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

void UFaerieItemStackContainer::BroadcastChange(const FFaerieInventoryTag Event)
{
	OnItemChangedNative.Broadcast(FFaerieItemProxy(this), Event);
	OnItemChanged.Broadcast(this, Event);
}

bool UFaerieItemStackContainer::IsOurItem(const FMassEntityHandle Item) const
{
	return ItemStack.Instance.GetMassEntityHandle() == Item;
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
	if (ValidStack.Instance != ItemStack.Instance)
	{
		// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
		ItemStack.StoredKey = KeyGen.NextKey();
		ItemStack.Instance = ValidStack.Instance;
		ItemStack.Copies = ValidStack.Copies;

		const FMassEntityHandle Entity = ItemStack.Instance.GetMassEntityHandle();

		// Take ownership of the new item if it's mutable
		if (ItemData::IsMutable(EntityManager, Entity))
		{
			Container::TakeOwnership(EntityManager, this, Entity);
		}

		// @Todo add way to disable these this

		// Find or create NetId for item.
		if (const FFaerieNetIDFragment* ExistingNetID = EntityManager.GetFragmentDataPtr<FFaerieNetIDFragment>(Entity))
		{
			ItemStack.NetID = ExistingNetID->NetID;
		}
		else
		{
			// @Todo better netId generation
			ItemStack.NetID = { static_cast<uint64>(FMath::FRand() * TNumericLimits<uint64>::Max()) };
			FInstancedStruct NetID;
			NetID.InitializeAs<FFaerieNetIDFragment>(FFaerieNetIDFragment{.NetID = ItemStack.NetID});
			EntityManager.AddFragmentInstanceListToEntity(Entity, MakeConstArrayView(&NetID, 1));
			EntityManager.AddSparseElementToEntity<FFaerieNeedsReplicationInit>(Entity);
		}

		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this, ItemStack.Instance, ValidStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));

		PostEvent(EntityManager, Event);

		BroadcastChange(Inventory::Tags::NewStack);
	}
	else
	{
		ItemStack.Copies += ValidStack.Copies;

		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeAddedCopies(this, ItemStack.Instance, ValidStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));

		PostEvent(EntityManager, Event);

		BroadcastChange(Inventory::Tags::AddedCopies);
	}
}

ItemData::FScopeProxy UFaerieItemStackContainer::GetView() const
{
	return ItemData::FScopeProxy(ItemStack.Instance, ItemStack.Copies, this);
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.HasValidInstance()) return false;

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = {
		.CheckType = EFaerieContainerAddStackCheckType::CouldEverAdd,
		.AddStackBehavior = EFaerieStorageAddStackBehavior::OnlyNewStacks
	};

	return AllowsAddition(EntityManager, MakeConstArrayView(&Proxy, 1), Args, true);
}

bool UFaerieItemStackContainer::CanSetInSlot(const FMassEntityManager& EntityManager,
	const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.HasValidInstance()) return false;

	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (Proxy.GetItemInstanceOrInvalid() != ItemStack.Instance)
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

	const ItemData::FScopeProxy StackProxy(Stack.Instance, Stack.Copies, nullptr);
	if (!CanSetInSlot(EntityManager, FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &StackProxy)))
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
	const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this, ItemStack.Instance, Copies, Reason,
		ItemStack.StoredKey, MakeConstArrayView(&CurrentAddress, 1), Copies == ItemStack.Copies);

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Event.EntryRemoved)
	{
		ItemStack.StoredKey = FFaerieEntryKey::InvalidKey;

		// Our local Item ptr must be nullptr before calling ReleaseOwnership so that extensions don't see that we have
		// this item. Extension logic is triggered by ReleaseOwnership as it calls ClearParentGroup on any subobjects.
		ItemStack = FFaerieStackContainerContent();

		// Release ownership of this item.
		if (Event.Instance.IsMutable())
		{
			Container::ReleaseOwnership(EntityManager, this, Event.Instance.GetMassEntityHandle());
		}
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	PostEvent(EntityManager, Event);

	BroadcastChange(Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		ItemStack.Instance.DestroyMassEntity(EntityManager);
		return FFaerieUnownedItemStack();
	}

	return FFaerieUnownedItemStack(Event.Instance, Event.Copies);
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
	return !ItemStack.Instance.IsEmpty() && ItemStack.Copies > 0;
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
	UE_LOGF(LogFaerieInventory, Log, "UFaerieItemStackContainer::OnRep_ItemStack")

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	// Case 1: Item changed by server.
	if (ItemStack.NetID != OldValue.NetID)
	{
		if (ItemStack.Copies == 0)
		{
			/*
			 * Note: the client doesn't know if the items were moved or deleted, so better to treat them as deleted downstream.
			 * From the clients perspective they may as well have been deleted, because the server may have moved them out of
			 * the sight of the client.
			 */
			const int32 RemovedCopies = OldValue.Copies;
			const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this, OldValue.Instance, RemovedCopies, Inventory::Tags::RemovalDeletion, OldValue.StoredKey, {}, true);
			PostEvent(EntityManager, Event);
			BroadcastChange(Inventory::Tags::RemovalDeletion);
		}
		else
		{
			UFaerieMassReplicationSubsystem* Replication = GetWorld()->GetSubsystem<UFaerieMassReplicationSubsystem>();
			if (TOptional<FMassEntityHandle> Item = Replication->ResolveNetId(ItemStack.NetID);
				Item.IsSet())
			{
				ItemStack.Instance = FFaerieItemInstance::FromEntity(Item.GetValue());

				const FFaerieAddress Address = GetCurrentAddress();
				const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeNewStack(this, ItemStack.Instance, ItemStack.Copies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
				PostEvent(EntityManager, Event);

				BroadcastChange(Inventory::Tags::NewStack);
			}
			else
			{
				// New item is not valid, until Ack'd to us by the replicator. Clear instance, until item is rebuilt.
				ItemStack.Instance = FFaerieItemInstance();
			}
		}
	}
	// Case 2: Same item, added to stack.
	else if (ItemStack.Copies > OldValue.Copies)
	{
		const int32 AddedCopies = ItemStack.Copies - OldValue.Copies;
		const FFaerieAddress Address = GetCurrentAddress();
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeAddedCopies(this, ItemStack.Instance, AddedCopies, ItemStack.StoredKey, MakeConstArrayView(&Address, 1));
		PostEvent(EntityManager, Event);

		BroadcastChange(Inventory::Tags::AddedCopies);
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
		const Container::FContainerEventPayload Event = Container::FContainerEventPayload::MakeRemoval(this, OldValue.Instance, RemovedCopies, Inventory::Tags::RemovalDeletion, OldValue.StoredKey, MakeConstArrayView(&Address, 1), !IsFilled());
		PostEvent(EntityManager, Event);
		BroadcastChange(Inventory::Tags::RemovedCopies);
	}
	else
	{
		UE_LOGF(LogFaerieInventory, Error, "Client received OnRep, but doesn't know what changed...")
	}
}