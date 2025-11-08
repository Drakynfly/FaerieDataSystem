// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStackContainer.h"

#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieInventoryLog.h"
#include "FaerieItem.h"
#include "FaerieItemStorageStatics.h"
#include "InventoryDataEnums.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStackContainer)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotSet, "Fae.Inventory.Set", "Event tag when item data is added to a container")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotTake, "Fae.Inventory.Take", "Event tag when item data is removed from a container")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotItemMutated, "Fae.Inventory.SlotItemMutated", "Event tag when the item in a container mutates")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotClientReplication, "Fae.Inventory.ClientReplication", "Event tag when item data is replicated to the client. Could have been caused by a Set/Take/Mutate from the server.")
}

namespace Faerie::Container
{
	/**
	 * Not really an iterator, this just exists to implement the Container::IIterator, so we can transparently interop
	 * with generic Container API that consumes iterators.
	 */
	class FStackContainerIteratorStub final : public IIterator
	{
	public:
		explicit FStackContainerIteratorStub(const UFaerieItemStackContainer* Stack)
		  : Stack(Stack) {}

		//~ Container::IIterator
		virtual FVirtualIterator Copy() const override
		{
			return FVirtualIterator(MakeUnique<FStackContainerIteratorStub>(Stack));
		}
		FORCEINLINE virtual void Advance() override { Stack = nullptr; }
		FORCEINLINE virtual FEntryKey ResolveKey() const override { return Stack->GetCurrentKey(); }
		FORCEINLINE virtual FFaerieAddress ResolveAddress() const override { return Stack->GetCurrentAddress(); }
		FORCEINLINE virtual const UFaerieItem* ResolveItem() const override { return Stack->GetItemObject(); }
		FORCEINLINE virtual bool IsValid() const override { return ::IsValid(Stack) && Stack->IsFilled(); }
		//~ Container::IIterator

	private:
		const UFaerieItemStackContainer* Stack;
	};

	class FStackContainerFilterStub final : public IFilter
	{
	public:
		explicit FStackContainerFilterStub(const UFaerieItemStackContainer* Stack)
		  : Stack(Stack)
		{
			Alive = Stack->IsFilled();
		}

	protected:
		virtual void Run_Impl(IItemDataFilter&& Filter) override
		{
			if (Alive && !Filter.Passes(Stack->GetItemObject()))
			{
				Alive = false;
			}
		}
		virtual void Run_Impl(IAddressFilter&& Filter) override
		{
			if (Alive && !Filter.Passes(Stack->GetCurrentAddress()))
			{
				Alive = false;
			}
		}
		virtual void Run_Impl(IEntryKeyFilter&& Filter) override
		{
			if (Alive && !Filter.Passes(Stack->GetCurrentKey()))
			{
				Alive = false;
			}
		}
		virtual void Run_Impl(ISnapshotFilter&& Filter) override
		{
			if (Alive)
			{
				FFaerieItemSnapshot Snapshot;
				Snapshot.Owner = Stack;
				Snapshot.ItemObject = Stack->GetItemObject();
				Snapshot.Copies = Stack->GetCopies();

				if (!Filter.Passes(Snapshot))
				{
					Alive = false;
				}
			}
		}
		virtual void Invert_Impl() override
		{
			Alive = !Alive;
		}

		virtual void Reset() override { Alive = true; }
		virtual int32 Num() const override { return Stack->GetStack(); }

		virtual FVirtualKeyIterator KeyRange() const override
		{
			if (Alive)
			{
				return FVirtualKeyIterator(FVirtualIterator(MakeUnique<FStackContainerIteratorStub>(Stack)));
			}
			return FVirtualKeyIterator{FVirtualIterator(nullptr)};
		}
		virtual FVirtualAddressIterator AddressRange() const override
		{
			if (Alive)
			{
				return FVirtualAddressIterator(FVirtualIterator(MakeUnique<FStackContainerIteratorStub>(Stack)));
			}
			return FVirtualAddressIterator{FVirtualIterator(nullptr)};
		}
		virtual FVirtualItemIterator ItemRange() const override
		{
			if (Alive)
			{
				return FVirtualItemIterator(FVirtualIterator(MakeUnique<FStackContainerIteratorStub>(Stack)));
			}
			return FVirtualItemIterator{FVirtualIterator(nullptr)};
		}
		virtual FVirtualConstItemIterator ConstItemRange() const override
		{
			if (Alive)
			{
				return FVirtualConstItemIterator(FVirtualIterator(MakeUnique<FStackContainerIteratorStub>(Stack)));
			}
			return FVirtualConstItemIterator(FVirtualIterator(nullptr));
		}

	private:
		const UFaerieItemStackContainer* Stack;
		bool Alive;
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
FInstancedStruct UFaerieItemStackContainer::MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	unimplemented();
	return FInstancedStruct();
}

void UFaerieItemStackContainer::LoadSaveData(const FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData)
{
	unimplemented();
}

bool UFaerieItemStackContainer::Contains(const FEntryKey Key) const
{
	return StoredKey == Key;
}

FFaerieItemStackView UFaerieItemStackContainer::View(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return ItemStack;
	}
	return FFaerieItemStackView();
}

FFaerieItemStackView UFaerieItemStackContainer::View() const
{
	return ItemStack;
}

FFaerieItemStack UFaerieItemStackContainer::Release(const FEntryKey Key, const int32 Copies)
{
	if (Key == StoredKey)
	{
		return TakeItemFromSlot(Copies);
	}
	return FFaerieItemStack();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy() const
{
	return this;
}

int32 UFaerieItemStackContainer::GetStack(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return ItemStack.Copies;
	}
	return 0;
}

bool UFaerieItemStackContainer::Contains(const FFaerieAddress Address) const
{
	return static_cast<int32>(Address.Address) == StoredKey.Value();
}

int32 UFaerieItemStackContainer::GetStack(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack.Copies;
	}
	return 0;
}

const UFaerieItem* UFaerieItemStackContainer::ViewItem(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack.Item;
	}
	return nullptr;
}

FFaerieItemStackView UFaerieItemStackContainer::ViewStack(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack;
	}
	return FFaerieItemStackView();
}

FFaerieItemProxy UFaerieItemStackContainer::Proxy(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return this;
	}
	return nullptr;
}

FFaerieItemStack UFaerieItemStackContainer::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (Contains(Address))
	{
		return TakeItemFromSlot(Copies);
	}
	return FFaerieItemStack();
}

TUniquePtr<Faerie::Container::IIterator> UFaerieItemStackContainer::CreateIterator(bool) const
{
	// Don't provide an iterator if we are empty...
	if (!IsFilled()) return nullptr;

	return MakeUnique<Faerie::Container::FStackContainerIteratorStub>(this);
}

TUniquePtr<Faerie::Container::IFilter> UFaerieItemStackContainer::CreateFilter(bool) const
{
	return MakeUnique<Faerie::Container::FStackContainerFilterStub>(this);
}

FEntryKey UFaerieItemStackContainer::FILTER_GetBaseKey(const FFaerieAddress Address) const
{
	if (Address == GetCurrentAddress())
	{
		return StoredKey;
	}
	return FEntryKey::InvalidKey;
}

TArray<FFaerieAddress> UFaerieItemStackContainer::FILTER_GetKeyAddresses(const FEntryKey Key) const
{
	if (Key == StoredKey)
	{
		return { GetCurrentAddress() };
	}
	return {};
}

int32 UFaerieItemStackContainer::GetStack() const
{
	return ItemStack.Copies;
}
//~ UFaerieItemContainerBase

//~ IFaerieItemDataProxy
const UFaerieItem* UFaerieItemStackContainer::GetItemObject() const
{
	return ItemStack.Item;
}

int32 UFaerieItemStackContainer::GetCopies() const
{
	return ItemStack.Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemStackContainer::GetItemOwner() const
{
	return const_cast<ThisClass*>(this);
}
//~ IFaerieItemDataProxy

//~ IFaerieItemOwnerInterface
FFaerieItemStack UFaerieItemStackContainer::Release(const FFaerieItemStackView Stack)
{
	if (Stack.Item == ItemStack.Item)
	{
		return TakeItemFromSlot(Stack.Copies);
	}
	return FFaerieItemStack();
}

bool UFaerieItemStackContainer::Possess(const FFaerieItemStack Stack)
{
	return SetItemInSlot(Stack);
}

void UFaerieItemStackContainer::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(Item, Token, EditTag);
	check(ItemStack.Item == Item);

	BroadcastChange(Faerie::Inventory::Tags::SlotItemMutated);
}
//~ IFaerieItemOwnerInterface

void UFaerieItemStackContainer::BroadcastChange(const FFaerieInventoryTag Event)
{
	OnItemChangedNative.Broadcast(this, Event);
	OnItemChanged.Broadcast(this, Event);
}

void UFaerieItemStackContainer::SetStoredItem_Impl(const FFaerieItemStack& Stack)
{
	Extensions->PreAddition(this, Stack);

	Faerie::Inventory::FEventLog Event;
	Event.Item = Stack.Item;
	Event.Amount = Stack.Copies;
	Event.Success = true;
	Event.Type = Faerie::Inventory::Tags::SlotSet;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Stack.Item != ItemStack.Item)
	{
		// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = KeyGen.NextKey();

		ItemStack = Stack;

		// Take ownership of the new item.
		Faerie::TakeOwnership(this, ItemStack.Item);
	}
	else
	{
		ItemStack.Copies += Stack.Copies;
	}

	Event.EntryTouched = StoredKey;

	Extensions->PostAddition(this, Event);

	BroadcastChange(Faerie::Inventory::Tags::SlotSet);
}

bool UFaerieItemStackContainer::CouldSetInSlot(const FFaerieItemStackView View) const
{
	if (!View.Item.IsValid()) return false;

	if (View.Copies > 1)
	{
		return false;
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions->AllowsAddition(this, MakeArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return false;
}

bool UFaerieItemStackContainer::CanSetInSlot(const FFaerieItemStackView View) const
{
	if (!View.Item.IsValid()) return false;

	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (View.Item != ItemStack.Item)
		{
			return false;
		}
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions->AllowsAddition(this, MakeArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieItemStackContainer::CanTakeFromSlot(const int32 Copies) const
{
	if (!ItemStack.IsValid()) return false;

	if (Copies != Faerie::ItemData::EntireStack &&
		ItemStack.Copies < Copies)
	{
		return false;
	}

	if (Extensions->AllowsRemoval(this, GetCurrentAddress(), Faerie::Inventory::Tags::SlotTake) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieItemStackContainer::SetItemInSlot(const FFaerieItemStack Stack)
{
	if (!CanSetInSlot(Stack))
	{
		UE_LOG(LogFaerieInventory, Warning,
			TEXT("Invalid request to set into container '%s'!"), *GetPathName())
		return false;
	}

	// If the above check passes, then either the Stack's item is the same as our's, or we are currently empty!
	SetStoredItem_Impl(Stack);
	return true;
}

FFaerieItemStack UFaerieItemStackContainer::TakeItemFromSlot(int32 Copies)
{
	if (!CanTakeFromSlot(Copies))
	{
		UE_LOG(LogFaerieInventory, Warning,
			TEXT("Invalid request to take item from container '%s'!"), *GetPathName())
		return FFaerieItemStack();
	}

	if (Copies > ItemStack.Copies)
	{
		UE_LOG(LogFaerieInventory, Error,
			TEXT("Cannot remove more copies from a container than what it contains. Container: '%s', Requested Copies: '%i', Contained: '%i' !"),
			*GetPathName(), Copies, ItemStack.Copies)
		return FFaerieItemStack();
	}

	if (Copies == Faerie::ItemData::EntireStack)
	{
		Copies = ItemStack.Copies;
	}

	Extensions->PreRemoval(this, StoredKey, Copies);

	Faerie::Inventory::FEventLog Event;
	Event.Item = ItemStack.Item;
	Event.Amount = Copies;
	Event.Success = true;
	Event.Type = Faerie::Inventory::Tags::SlotTake;
	Event.EntryTouched = StoredKey;

	const FFaerieItemStack OutStack{ItemStack.Item, Copies};

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Copies == ItemStack.Copies)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = FEntryKey::InvalidKey;

		// Our local Item ptr must be nullptr before calling ReleaseOwnership // @todo why??
		ItemStack = FFaerieItemStack();

		// Release ownership of this item.
		Faerie::ReleaseOwnership(this, OutStack.Item);
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	Extensions->PostRemoval(this, Event);

	BroadcastChange(Faerie::Inventory::Tags::SlotTake);

	return OutStack;
}

FEntryKey UFaerieItemStackContainer::GetCurrentKey() const
{
	return StoredKey;
}

FFaerieAddress UFaerieItemStackContainer::GetCurrentAddress() const
{
	return FFaerieAddress(StoredKey.Value());
}

bool UFaerieItemStackContainer::IsFilled() const
{
	return IsValid(ItemStack.Item) && ItemStack.Copies > 0;
}

void UFaerieItemStackContainer::OnRep_ItemStack()
{
	BroadcastChange(Faerie::Inventory::Tags::SlotClientReplication);
}