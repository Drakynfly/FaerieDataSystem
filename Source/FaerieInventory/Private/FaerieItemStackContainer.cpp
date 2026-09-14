// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStackContainer.h"
#include "EntityManagerHelpers.h"

#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieItemOwnership.h"
#include "FaerieStorageEnums.h"
#include "FaerieUnownedItemStack.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"

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
			UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Stack->GetItemInstance(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			UE_REWRITE virtual const IFaerieItemOwnerInterface* GetItemOwner() const override { return Stack; }
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
			UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Stack->GetItemInstance(); }
			UE_REWRITE virtual int32 GetCopies() const override { return Stack->GetStackCopies(); }
			UE_REWRITE virtual const IFaerieItemOwnerInterface* GetItemOwner() const override { return Stack; }
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

void UFaerieItemStackContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// State members are push based
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemStack, SharedParams)
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StoredKey, SharedParams)
}

FInstancedStruct UFaerieItemStackContainer::MakeSaveData(const Container::FSaveParams Params) const
{
	FFaerieSimpleItemStackSaveData SlotSaveData;
	MakeSaveData(SlotSaveData, Params);
	return FInstancedStruct::Make(SlotSaveData);
}

void UFaerieItemStackContainer::LoadSaveData(const FConstStructView ItemData, const Container::FLoadParams Params)
{
	const FFaerieSimpleItemStackSaveData* SaveData = ItemData.GetPtr<const FFaerieSimpleItemStackSaveData>();
	if (!SaveData)
	{
		UE_LOGF(LogFaerieInventory, Error, "Invalid data to load from. Must be FFaerieSimpleItemStackSaveData.")
		return;
	}

	LoadSaveData(*SaveData, Params);
}

bool UFaerieItemStackContainer::Contains(const FFaerieAddress Address) const
{
	return IsOurAddress(Address);
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

bool UFaerieItemStackContainer::Possess(const FFaerieUnownedItemStack& Stack)
{
	return SetItemInSlot(Stack);
}

void UFaerieItemStackContainer::DestroyStack(const FFaerieEntryKey Key, const int32 Copies)
{
	if (IsOurKey(Key))
	{
		TakeItemFromSlot(Copies, Inventory::Tags::RemovalDeletion);
	}
}

void UFaerieItemStackContainer::DestroyStack(const FFaerieAddress Address, const int32 Copies)
{
	if (IsOurAddress(Address))
	{
		TakeItemFromSlot(Copies, Inventory::Tags::RemovalDeletion);
	}
}

void UFaerieItemStackContainer::DestroyStack(const FFaerieItemProxy& Proxy, const int32 Copies)
{
	if (Proxy.GetProxyObject() == this && IsFilled())
	{
		TakeItemFromSlot(Copies, Inventory::Tags::RemovalDeletion);
	}
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(const FFaerieEntryKey Key, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (IsOurKey(Key))
	{
		return TakeItemFromSlot(Copies, Reason);
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(const FFaerieAddress Address, const int32 Copies, const FFaerieInventoryTag Reason)
{
	if (IsOurAddress(Address))
	{
		return TakeItemFromSlot(Copies, Reason);
	}
	return NullOpt;
}

bool UFaerieItemStackContainer::CanPossess(const FFaerieItemProxy& Proxy) const
{
	return CanSetInSlot(Proxy);
}

bool UFaerieItemStackContainer::CanRelease(const FFaerieItemProxy& Proxy, const FFaerieInventoryTag Reason) const
{
	if (Proxy.GetProxyObject() == this)
	{
		return CanTakeFromSlot(Proxy.GetCopies(), Reason);
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

IFaerieItemOwnerInterface* UFaerieItemStackContainer::GetItemOwner() const
{
	return const_cast<ThisClass*>(this);
}

FFaerieAddress UFaerieItemStackContainer::Proxy_GetAddress() const
{
	return GetCurrentAddress();
}

FFaerieItemNetworkHandle UFaerieItemStackContainer::Proxy_GetNetworkHandle() const
{
	return GetNetworkHandle();
}

void UFaerieItemStackContainer::OnItemDataChanged(const FFaerieItemInstance& Instance, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(Instance, EditTag);
	check(ItemStack.Instance == Instance);

	BroadcastChange(Inventory::Tags::ReplicationEdit);
}

void UFaerieItemStackContainer::MakeSaveData(FFaerieSimpleItemStackSaveData& SaveData, const Container::FSaveParams Params) const
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	if (Params.ExportItemData)
	{
		if (StoredKey.IsValid())
        {
        	SaveData.ItemObject = ItemStack.Instance.GetItemPtr();
        	SaveData.Copies = ItemStack.Copies;
        	SaveData.ExportData = ExportItemData(EntityManager, ItemStack.Instance);
        }
	}

	if (Params.ExportExtensionData)
	{
		RavelExtensionData(SaveData.ExtensionData);
	}
}

void UFaerieItemStackContainer::LoadSaveData(const FFaerieSimpleItemStackSaveData& SaveData, const Container::FLoadParams Params)
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
			TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
		}
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	if (SaveData.Copies > 0)
	{
		// Rebuild instance from save data
		const FFaerieItemInstance Instance = ImportItemData(EntityManager, SaveData.ItemObject, SaveData.ExportData);

		if (Container::ValidateItemData(Instance) &&
			SaveData.Copies > 0)
		{
			// If it validated, store in slot.
			const TValid<FFaerieUnownedItemStack> Stack(Instance, SaveData.Copies);
			SetStoredItem_Impl(Stack);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOGF(LogFaerieInventory, Error, "Loading content for stack container '%ls' failed. Stored item has been reset!", *GetPathName())
			MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieItemStackContainer, StoredKey, this);
			StoredKey = FFaerieEntryKey();
		}
	}

	UnravelExtensionData(SaveData.ExtensionData);
	InitializeExtensions();
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

bool UFaerieItemStackContainer::IsOurKey(const FFaerieEntryKey Key) const
{
	return StoredKey == Key;
}

bool UFaerieItemStackContainer::IsOurAddress(const FFaerieAddress Address) const
{
	return static_cast<int32>(Address.Address) == StoredKey.Value();
}

void UFaerieItemStackContainer::SetStoredItem_Impl(const TValid<FFaerieUnownedItemStack>& NewItemStack)
{
	const FFaerieItemInstance Instance = ValidGet(NewItemStack).Instance;
	const int32 Copies = ValidGet(NewItemStack).Copies;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Instance != ItemStack.Instance)
	{
		// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = KeyGen.NextKey();

		ItemStack.Instance = Instance;
		ItemStack.Copies = Copies;

		// Take ownership of the new item if it's mutable
		if (ItemStack.Instance.IsMutable())
		{
			auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
			Container::TakeOwnership(EntityManager, this, ItemStack.Instance);
		}
	}
	else
	{
		ItemStack.Copies += Copies;
	}

	const FFaerieAddress CurrentAddress = GetCurrentAddress();
	const Inventory::FEventData Event(Instance, Copies, StoredKey, MakeConstArrayView(&CurrentAddress, 1));

	PostEvent(Event, Inventory::Tags::Addition);

	BroadcastChange(Inventory::Tags::Addition);
}

ItemData::FScopeProxy UFaerieItemStackContainer::GetView() const
{
	return ItemData::FScopeProxy(ItemStack.Instance, ItemStack.Copies, this);
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.IsValid()) return false;

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = {
		.CheckType = EFaerieContainerAddStackCheckType::CouldEverAdd,
		.AddStackBehavior = EFaerieStorageAddStackBehavior::OnlyNewStacks
	};

	return AllowsAddition(MakeConstArrayView(&Proxy, 1), Args, true);
}

bool UFaerieItemStackContainer::CanSetInSlot(const FFaerieItemProxy& Proxy) const
{
	if (!Proxy.IsValid()) return false;

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

	return AllowsAddition(MakeConstArrayView(&Proxy, 1), Args, true);
}

bool UFaerieItemStackContainer::CanTakeFromSlot(const int32 Copies, const FFaerieInventoryTag Reason) const
{
	if (!IsFilled()) return false;

	if (Copies != ItemData::EntireStack &&
		ItemStack.Copies < Copies)
	{
		return false;
	}

	Container::FStackContainerIteratorStub ViewStub(this);
	return AllowsRemoval(&ViewStub, Reason, true);
}

bool UFaerieItemStackContainer::SetItemInSlot(const FFaerieUnownedItemStack& Stack)
{
	const ItemData::FScopeProxy StackProxy(Stack.Instance, Stack.Copies, nullptr);
	if (!CanSetInSlot(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &StackProxy)))
	{
		UE_LOGF(LogFaerieInventory, Warning,
			"Invalid request to set into container '%ls'!", *GetPathName())
		return false;
	}

	// If the above check passes, then either the Stack's item is the same as ours, or we are currently empty!
	SetStoredItem_Impl(Stack);
	return true;
}

#if WITH_EDITOR
void UFaerieItemStackContainer::SetItemInSlot_Editor(const FFaerieUnownedItemStack& Stack)
{
	// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
	StoredKey = KeyGen.NextKey();

	ItemStack.Instance = Stack.Instance;
	ItemStack.Copies = Stack.Copies;
	(void)MarkPackageDirty();
}
#endif

FFaerieUnownedItemStack UFaerieItemStackContainer::TakeItemFromSlot(int32 Copies, const FFaerieInventoryTag Reason)
{
	if (!CanTakeFromSlot(Copies, Reason))
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
	Inventory::FEventData Event(ItemStack.Instance, Copies, StoredKey, MakeConstArrayView(&CurrentAddress, 1));

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Copies == ItemStack.Copies)
	{
		Event.EntryRemoved = true;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = FFaerieEntryKey::InvalidKey;

		// Our local Item ptr must be nullptr before calling ReleaseOwnership so that extensions don't see that we have
		// this item. Extension logic is triggered by ReleaseOwnership as it calls ClearParentGroup on any subobjects.
		ItemStack = FFaerieStackContainerContent();

		// Release ownership of this item.
		if (Event.Instance.IsMutable())
		{
			auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
			Container::ReleaseOwnership(EntityManager, this, Event.Instance);
		}
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	PostEvent(Event, Reason);

	BroadcastChange(Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		ItemStack.Instance.DestroyMassEntity(EntityManager);
		return FFaerieUnownedItemStack();
	}

	return FFaerieUnownedItemStack(Event.Instance, Event.Copies);
}

#if WITH_EDITOR
void UFaerieItemStackContainer::ClearStackInSlot_Editor()
{
	StoredKey = FFaerieEntryKey::InvalidKey;
	ItemStack = FFaerieStackContainerContent();
	(void)MarkPackageDirty();
}
#endif

int32 UFaerieItemStackContainer::GetStackCopies() const
{
	return ItemStack.Copies;
}

FFaerieEntryKey UFaerieItemStackContainer::GetCurrentKey() const
{
	return StoredKey;
}

FFaerieAddress UFaerieItemStackContainer::GetCurrentAddress() const
{
	return FFaerieAddress(StoredKey.Value());
}

FFaerieItemNetworkHandle UFaerieItemStackContainer::GetNetworkHandle() const
{
	return FFaerieItemNetworkHandle{const_cast<ThisClass*>(this), GetCurrentAddress()};
}

bool UFaerieItemStackContainer::IsFilled() const
{
	return !ItemStack.Instance.IsEmpty() && ItemStack.Copies > 0;
}

void UFaerieItemStackContainer::OnRep_ItemStack(const FFaerieStackContainerContent& OldValue)
{
	if (ItemStack.Copies > OldValue.Copies)
	{
		BroadcastChange(Inventory::Tags::Addition);
	}
	else if (ItemStack.Copies < OldValue.Copies)
	{
		/*
		 * Note: the client doesn't know if the items were moved or deleted, so better to treat them as deleted downstream.
		 * From the clients perspective they may as well have been deleted, because the server may have moved them out of
		 * the sight of the client.
		 */
		BroadcastChange(Inventory::Tags::RemovalDeletion);
	}
	else if (ItemStack.Instance != OldValue.Instance)
	{
		// @todo: the broadcast system doesn't differentiate between adding to a new stack versus an existing one.
		BroadcastChange(Inventory::Tags::Addition);
	}
	else
	{
		UE_LOGF(LogFaerieInventory, Error, "Client received OnRep, but doesn't know what changed...")
	}
}