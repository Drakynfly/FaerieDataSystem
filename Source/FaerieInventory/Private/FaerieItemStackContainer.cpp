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

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotItemMutated, "Fae.Inventory.SlotItemMutated", "Event tag when the item in a container mutates")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotClientReplication, "Fae.Inventory.ClientReplication", "Event tag when item data is replicated to the client. Could have been caused by a Set/Take/Mutate from the server.")
}

namespace Faerie::Container
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
		UE_REWRITE virtual ItemData::FReference ResolveItem() const override { return Stack->GetItemInstance().GetValue(); }
		UE_REWRITE virtual int32 ResolveCopies() const override { return Stack->GetStackCopies(); }
		UE_REWRITE virtual const IFaerieItemOwnerInterface* ResolveOwner() const override { return Stack; }
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
		UE_REWRITE virtual ItemData::FReference ResolveItem() const override { return Stack->GetItemInstance().GetValue(); }
		UE_REWRITE virtual int32 ResolveCopies() const override { return Stack->GetStackCopies(); }
		UE_REWRITE virtual const IFaerieItemOwnerInterface* ResolveOwner() const override { return Stack; }
		//~ ItemData::IViewBase

		//~ Container::IAddressIterator
		UE_REWRITE virtual void Advance() override { Stack = nullptr; }
		UE_REWRITE virtual bool IsValid() const override { return ::IsValid(Stack) && Stack->IsFilled(); }
		//~ Container::IAddressIterator

	private:
		const UFaerieItemStackContainer* Stack;
	};
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

//~ UFaerieItemContainerBase
FInstancedStruct UFaerieItemStackContainer::MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const
{
	RavelExtensionData(ExtensionData);

	FFaerieSimpleItemStackSaveData SlotSaveData;
	if (StoredKey.IsValid())
	{
		SlotSaveData.ItemObject = ItemStack.Instance.GetItemPtr();
		SlotSaveData.Copies = ItemStack.Copies;
		SlotSaveData.ExportData = ExportItemData(ItemData::FRequireEntityManager(this), ItemStack.Instance);
	}
	return FInstancedStruct::Make(SlotSaveData);
}

void UFaerieItemStackContainer::LoadSaveData(const FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	const FFaerieSimpleItemStackSaveData* SaveData = ItemData.GetPtr<const FFaerieSimpleItemStackSaveData>();
	if (!SaveData)
	{
		return;
	}

	// Clear any current content.
	if (IsFilled())
	{
		TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}

	if (SaveData->Copies > 0)
	{
		// Rebuild instance from save data
		const FFaerieItemInstance Instance = ImportItemData(ItemData::FRequireEntityManager(this), SaveData->ItemObject, SaveData->ExportData);

		if (Container::ValidateItemData(Instance) &&
			SaveData->Copies > 0)
		{
			// If it validated, store in slot.
			const FFaerieItemDataView DataView(Instance, SaveData->Copies, nullptr);
			SetStoredItem_Impl(DataView);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOG(LogFaerieInventory, Error, TEXT("Loading content for stack container '%s' failed. Stored item has been reset!"), *GetPathName())
			MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieItemStackContainer, StoredKey, this);
			StoredKey = FFaerieEntryKey();
		}
	}

	if (ExtensionData.IsValid())
	{
		UnravelExtensionData(ExtensionData);
	}
}

bool UFaerieItemStackContainer::Contains(const FFaerieAddress Address) const
{
	return IsOurAddress(Address);
}

FFaerieItemInstance UFaerieItemStackContainer::ViewInstance(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return ItemStack.Instance;
	}
	return FFaerieItemInstance();
}

FFaerieItemInstance UFaerieItemStackContainer::ViewInstance(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return ItemStack.Instance;
	}
	return FFaerieItemInstance();
}

FFaerieItemDataView UFaerieItemStackContainer::ViewEntry(const FFaerieEntryKey Key) const
{
	if (IsOurKey(Key))
	{
		return GetView();
	}
	return FFaerieItemDataView();
}

FFaerieItemDataView UFaerieItemStackContainer::ViewAddress(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return GetView();
	}
	return FFaerieItemDataView();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FFaerieAddress Address) const
{
	if (IsOurAddress(Address))
	{
		return FFaerieItemProxy(this);
	}
	return FFaerieItemProxy();
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

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(const FFaerieEntryKey Key, const int32 Copies)
{
	if (IsOurKey(Key))
	{
		return TakeItemFromSlot(Copies, Inventory::Tags::RemovalMoving);
	}
	return NullOpt;
}

TOptional<FFaerieUnownedItemStack> UFaerieItemStackContainer::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (IsOurAddress(Address))
	{
		return TakeItemFromSlot(Copies, Inventory::Tags::RemovalMoving);
	}
	return NullOpt;
}

bool UFaerieItemStackContainer::CanPossess(const FFaerieItemDataView& View) const
{
	return CanSetInSlot(View);
}

void UFaerieItemStackContainer::GetAllAddresses(TArray<FFaerieAddress>& Addresses) const
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

//~ UFaerieItemContainerBase

//~ IFaerieItemDataProxy
TOptional<FFaerieItemInstance> UFaerieItemStackContainer::GetItemInstance() const
{
	if (ItemStack.Instance.IsValid())
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

//~ IFaerieItemDataProxy

//~ IFaerieItemOwnerInterface

FFaerieAddress UFaerieItemStackContainer::GetAddress() const
{
	return GetCurrentAddress();
}

bool UFaerieItemStackContainer::Possess(const FFaerieUnownedItemStack& View)
{
	return SetItemInSlot(View);
}

void UFaerieItemStackContainer::DestroyStack(const FFaerieItemProxy& Proxy, const int32 Copies)
{
	if (Proxy.GetProxyObject() == this && IsFilled())
	{
		TakeItemFromSlot(Copies, Inventory::Tags::RemovalDeletion);
	}
}

void UFaerieItemStackContainer::OnItemDataChanged(const ItemData::FMutableReference& Instance, const TNotNull<const UScriptStruct*> Struct, const FGameplayTag EditTag)
{
	Super::OnItemDataChanged(Instance, Struct, EditTag);
	check(ItemStack.Instance == Instance);

	BroadcastChange(Inventory::Tags::SlotItemMutated);
}
//~ IFaerieItemOwnerInterface

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

void UFaerieItemStackContainer::SetStoredItem_Impl(const ItemData::FValidatedDataView View)
{
	Extensions::FGroupAPI::PreAddition(Extensions, this, View.DataView);

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (View->GetInstance() != ItemStack.Instance)
	{
		// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = KeyGen.NextKey();

		ItemStack.Instance = View->GetInstance();
		ItemStack.Copies = View->GetCopies();

		// Take ownership of the new item if it's mutable
		if (ItemStack.Instance.IsMutable())
		{
			Container::TakeOwnership(ItemData::FRequireEntityManager(this), this, ItemStack.Instance);
		}
	}
	else
	{
		ItemStack.Copies += View->GetCopies();
	}

	const FFaerieAddress CurrentAddress = GetCurrentAddress();
	const Inventory::FEventData Event(View->GetInstance(), View->GetCopies(), StoredKey, MakeConstArrayView(&CurrentAddress, 1));

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Inventory::Tags::Addition);

	BroadcastChange(Inventory::Tags::Addition);
}

FFaerieItemDataView UFaerieItemStackContainer::GetView() const
{
	return FFaerieItemDataView(ItemStack.Instance, ItemStack.Copies, this);
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FFaerieItemDataView& View) const
{
	if (!View.IsValid()) return false;

	const int32 ViewCopies = View.GetCopies();
	if (ViewCopies > 1)
	{
		return false;
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions::FGroupAPI::AllowsAddition(Extensions, this, MakeConstArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return false;
}

bool UFaerieItemStackContainer::CanSetInSlot(const FFaerieItemDataView& View) const
{
	if (!View.IsValid()) return false;

	const FFaerieItemInstance Instance = View.GetInstance();
	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (Instance != ItemStack.Instance)
		{
			return false;
		}
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions::FGroupAPI::AllowsAddition(Extensions, this, MakeConstArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieItemStackContainer::CanTakeFromSlot(const int32 Copies, const FFaerieInventoryTag Reason) const
{
	if (!IsFilled()) return false;

	if (Copies != ItemData::EntireStack &&
		ItemStack.Copies < Copies)
	{
		return false;
	}

	const Container::FStackContainerIteratorStub ViewStub(this);
	if (Extensions::FGroupAPI::AllowsRemoval(Extensions, this, ViewStub, Reason) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieItemStackContainer::SetItemInSlot(const FFaerieUnownedItemStack& Stack)
{
	const FFaerieItemDataView StackView(Stack.Instance, Stack.Copies, nullptr);
	if (!CanSetInSlot(StackView))
	{
		UE_LOG(LogFaerieInventory, Warning,
			TEXT("Invalid request to set into container '%s'!"), *GetPathName())
		return false;
	}

	// If the above check passes, then either the Stack's item is the same as ours, or we are currently empty!
	SetStoredItem_Impl(StackView);
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
		UE_LOG(LogFaerieInventory, Warning,
			TEXT("Invalid request to take item from container '%s'!"), *GetPathName())
		return FFaerieUnownedItemStack();
	}

	if (Copies > ItemStack.Copies)
	{
		UE_LOG(LogFaerieInventory, Error,
			TEXT("Cannot remove more copies from a container than what it contains. Container: '%s', Requested Copies: '%i', Contained: '%i' !"),
			*GetPathName(), Copies, ItemStack.Copies)
		return FFaerieUnownedItemStack();
	}

	if (Copies == ItemData::EntireStack)
	{
		Copies = ItemStack.Copies;
	}

	const Container::FEntryContainerIteratorStub ViewStub(this);
	Extensions::FGroupAPI::PreRemoval(Extensions, this, ViewStub, Copies);

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
			Container::ReleaseOwnership(ItemData::FRequireEntityManager(this), this, Event.Instance);
		}
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	Extensions::FGroupAPI::PostEvent(Extensions, this, Event, Reason);

	BroadcastChange(Reason);

	// Destroy the mass entity, if this stack is being deleted.
	if (Reason == Inventory::Tags::RemovalDeletion && Event.EntryRemoved)
	{
		ItemStack.Instance.DestroyMassEntity(ItemData::FRequireEntityManager(this));
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
	return ItemStack.Instance.IsValid() && ItemStack.Copies > 0;
}

void UFaerieItemStackContainer::OnRep_ItemStack()
{
	BroadcastChange(Inventory::Tags::SlotClientReplication);
}