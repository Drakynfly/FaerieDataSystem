// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentSlot.h"

#include "FaerieEquipmentSlotDescription.h"
#include "FaerieAssetInfo.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerIterator.h"
#include "FaerieEquipmentLog.h"
#include "FaerieItem.h"
#include "FaerieItemStorageStatics.h"
#include "FaerieItemTemplate.h"
#include "InventoryDataEnums.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"
#include "Tokens/FaerieChildSlotToken.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentSlot)

namespace Faerie::Equipment::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotSet, "Fae.Inventory.Set", "Event tag when item data is added to a slot")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotTake, "Fae.Inventory.Take", "Event tag when item data is removed from a slot")
}

namespace Faerie
{
	/**
	 * Not really an iterator, this just exists to implement the IContainerIterator, so we can transparently interop
	 * with generic Container API that consumes iterators.
	 */
	class FSlotIteratorStub final : public IContainerIterator
	{
	public:
		explicit FSlotIteratorStub(const UFaerieEquipmentSlot* Slot)
		  : Slot(Slot) {}

		//~ IContainerIterator
		virtual FDefaultIteratorStorage Copy() const override
		{
			return FDefaultIteratorStorage(MakeUnique<FSlotIteratorStub>(Slot));
		}
		FORCEINLINE virtual void Advance() override { Slot = nullptr; }
		FORCEINLINE virtual FEntryKey ResolveKey() const override { return Slot->GetCurrentKey(); }
		FORCEINLINE virtual FFaerieAddress ResolveAddress() const override { return Slot->GetCurrentAddress(); }
		FORCEINLINE virtual const UFaerieItem* ResolveItem() const override { return Slot->GetItemObject(); }
		FORCEINLINE virtual bool IsValid() const override { return ::IsValid(Slot) && Slot->IsFilled(); }
		FORCEINLINE virtual bool Equals(const TUniquePtr<IContainerIterator>& Other) const override
		{
			return Slot == reinterpret_cast<FSlotIteratorStub*>(Other.Get())->Slot;
		}
		//~ IContainerIterator

	private:
		const UFaerieEquipmentSlot* Slot;
	};

	class FSlotFilterStub final : public IContainerFilter
	{
	public:
		explicit FSlotFilterStub(const UFaerieEquipmentSlot* Slot)
		  : Slot(Slot)
		{
			Alive = Slot->IsFilled();
		}

	protected:
		virtual void Run_Impl(IItemDataFilter&& Filter) override
		{
			if (Alive && !Filter.Passes(Slot->GetItemObject()))
			{
				Alive = false;
			}
		}
		virtual void Run_Impl(IEntryKeyFilter&& Filter) override
		{
			if (Alive && !Filter.Passes(Slot->GetCurrentKey()))
			{
				Alive = false;
			}
		}
		virtual void Run_Impl(ISnapshotFilter&& Filter) override
		{
			if (Alive)
			{
				FFaerieItemSnapshot Snapshot;
				Snapshot.Owner = Slot;
				Snapshot.ItemObject = Slot->GetItemObject();
				Snapshot.Copies = Slot->GetCopies();

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
		virtual int32 Num() const override { return Slot->GetStack(); }

		virtual FDefaultKeyIterator KeyRange() const override
		{
			if (Alive)
			{
				return FDefaultKeyIterator(FDefaultIteratorStorage(MakeUnique<FSlotIteratorStub>(Slot)));
			}
			return FDefaultKeyIterator{FDefaultIteratorStorage(nullptr)};
		}
		virtual FDefaultAddressIterator AddressRange() const override
		{
			if (Alive)
			{
				return FDefaultAddressIterator(FDefaultIteratorStorage(MakeUnique<FSlotIteratorStub>(Slot)));
			}
			return FDefaultAddressIterator{FDefaultIteratorStorage(nullptr)};
		}
		virtual FDefaultItemIterator ItemRange() const override
		{
			if (Alive)
			{
				return FDefaultItemIterator(FDefaultIteratorStorage(MakeUnique<FSlotIteratorStub>(Slot)));
			}
			return FDefaultItemIterator{FDefaultIteratorStorage(nullptr)};
		}
		virtual FDefaultConstItemIterator ConstItemRange() const override
		{
			if (Alive)
			{
				return FDefaultConstItemIterator(FDefaultIteratorStorage(MakeUnique<FSlotIteratorStub>(Slot)));
			}
			return FDefaultConstItemIterator(FDefaultIteratorStorage(nullptr));
		}

	private:
		const UFaerieEquipmentSlot* Slot;
		bool Alive;
	};
}

void UFaerieEquipmentSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Config only needs to replicate once
	DOREPLIFETIME_CONDITION(ThisClass, Config, COND_InitialOnly);

	// State members are push based
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemStack, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StoredKey, SharedParams);
}

FInstancedStruct UFaerieEquipmentSlot::MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	return FInstancedStruct::Make(MakeSlotData(ExtensionData));
}

void UFaerieEquipmentSlot::LoadSaveData(const FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData)
{
	const FFaerieEquipmentSlotSaveData* SlotSaveData = ItemData.GetPtr<const FFaerieEquipmentSlotSaveData>();
	if (!SlotSaveData)
	{
		return;
	}

	LoadSlotData(*SlotSaveData, ExtensionData);
}

//~ UFaerieItemContainerBase
bool UFaerieEquipmentSlot::Contains(const FEntryKey Key) const
{
	return StoredKey == Key;
}

FFaerieItemStackView UFaerieEquipmentSlot::View(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return ItemStack;
	}
	return FFaerieItemStackView();
}

FFaerieItemStackView UFaerieEquipmentSlot::View() const
{
	return ItemStack;
}

FFaerieItemStack UFaerieEquipmentSlot::Release(const FEntryKey Key, const int32 Copies)
{
	if (Key == StoredKey)
	{
		return TakeItemFromSlot(Copies);
	}
	return FFaerieItemStack();
}

FFaerieItemProxy UFaerieEquipmentSlot::Proxy() const
{
	return this;
}

int32 UFaerieEquipmentSlot::GetStack(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return ItemStack.Copies;
	}
	return 0;
}

bool UFaerieEquipmentSlot::Contains(const FFaerieAddress Address) const
{
	return static_cast<int32>(Address.Address) == StoredKey.Value();
}

int32 UFaerieEquipmentSlot::GetStack(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack.Copies;
	}
	return 0;
}

const UFaerieItem* UFaerieEquipmentSlot::ViewItem(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack.Item;
	}
	return nullptr;
}

FFaerieItemStackView UFaerieEquipmentSlot::ViewStack(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return ItemStack;
	}
	return FFaerieItemStackView();
}

FFaerieItemProxy UFaerieEquipmentSlot::Proxy(const FFaerieAddress Address) const
{
	if (Contains(Address))
	{
		return this;
	}
	return nullptr;
}

FFaerieItemStack UFaerieEquipmentSlot::Release(const FFaerieAddress Address, const int32 Copies)
{
	if (Contains(Address))
	{
		return TakeItemFromSlot(Copies);
	}
	return FFaerieItemStack();
}

TUniquePtr<Faerie::IContainerIterator> UFaerieEquipmentSlot::CreateIterator() const
{
	// Don't provide an iterator if we are empty...
	if (!IsFilled()) return nullptr;

	return MakeUnique<Faerie::FSlotIteratorStub>(this);
}

TUniquePtr<Faerie::IContainerFilter> UFaerieEquipmentSlot::CreateFilter(bool) const
{
	return MakeUnique<Faerie::FSlotFilterStub>(this);
}

FFaerieEquipmentSlotSaveData UFaerieEquipmentSlot::MakeSlotData(TMap<FGuid, FInstancedStruct>& ExtensionData) const
{
	RavelExtensionData(ExtensionData);

	FFaerieEquipmentSlotSaveData SlotSaveData;
	SlotSaveData.Config = Config;
	SlotSaveData.StoredKey = StoredKey;
	if (StoredKey.IsValid())
	{
		SlotSaveData.ItemStack = ItemStack;
	}
	return SlotSaveData;
}

void UFaerieEquipmentSlot::LoadSlotData(const FFaerieEquipmentSlotSaveData& SlotData, UFaerieItemContainerExtensionData* ExtensionData)
{
	// @todo should check all Config members, not just SlotID
	if (!ensure(Config.SlotID == SlotData.Config.SlotID))
	{
		return;
	}

	// Clear any current content.
	if (IsFilled())
	{
		TakeItemFromSlot(-1);
	}

	// Cannot change Config here, as it only replicates once!

	if (SlotData.StoredKey.IsValid())
	{
		KeyGen.SetPosition(SlotData.StoredKey);

		const FFaerieItemStack& LoadedItemStack = SlotData.ItemStack;
		if (Faerie::ValidateItemData(LoadedItemStack.Item) &&
			LoadedItemStack.Copies > 0)
		{
			// We do need to ClearOwnership here, as whatever loaded the data may have parented the items automatically.
			Faerie::ClearOwnership(LoadedItemStack.Item);
			SetItemInSlot_Impl(LoadedItemStack);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOG(LogFaerieEquipment, Error, TEXT("Loading content for slot '%s' failed. Slot has been emptied!"), *Config.SlotID.ToString())
			MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
			StoredKey = FEntryKey();
		}
	}

	UnravelExtensionData(ExtensionData);
}

int32 UFaerieEquipmentSlot::GetStack() const
{
	return ItemStack.Copies;
}
//~ UFaerieItemContainerBase

//~ IFaerieItemDataProxy
const UFaerieItem* UFaerieEquipmentSlot::GetItemObject() const
{
	return ItemStack.Item;
}

int32 UFaerieEquipmentSlot::GetCopies() const
{
	return ItemStack.Copies;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieEquipmentSlot::GetItemOwner() const
{
	return const_cast<ThisClass*>(this);
}
//~ IFaerieItemDataProxy

//~ IFaerieItemOwnerInterface
FFaerieItemStack UFaerieEquipmentSlot::Release(const FFaerieItemStackView Stack)
{
	if (Stack.Item == ItemStack.Item)
	{
		return TakeItemFromSlot(Stack.Copies);
	}
	return FFaerieItemStack();
}

bool UFaerieEquipmentSlot::Possess(const FFaerieItemStack Stack)
{
	return SetItemInSlot(Stack);
}

void UFaerieEquipmentSlot::OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(Item, Token, EditTag);
	check(ItemStack.Item == Item);

	BroadcastDataChange();
}

//~ IFaerieItemOwnerInterface

void UFaerieEquipmentSlot::BroadcastChange()
{
	OnItemChangedNative.Broadcast(this);
	OnItemChanged.Broadcast(this);
}

void UFaerieEquipmentSlot::BroadcastDataChange()
{
	OnItemDataChangedNative.Broadcast(this);
	OnItemDataChanged.Broadcast(this);
}

void UFaerieEquipmentSlot::SetItemInSlot_Impl(const FFaerieItemStack& Stack)
{
	Extensions->PreAddition(this, Stack);

	Faerie::Inventory::FEventLog Event;
	Event.Item = Stack.Item;
	Event.Amount = Stack.Copies;
	Event.Success = true;
	Event.Type = Faerie::Equipment::Tags::SlotSet;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	// Increment key when stored item changes. This is only going to happen if ItemStack.Item is currently nullptr.
	if (Stack.Item != ItemStack.Item)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = KeyGen.NextKey();

		ItemStack = Stack;

		// Take ownership of the new item.
		Faerie::TakeOwnership(this, ItemStack.Item);

		Event.EntryTouched = StoredKey;
	}
	else
	{
		ItemStack.Copies += Stack.Copies;

		Event.EntryTouched = StoredKey;
	}

	Extensions->PostAddition(this, Event);

	BroadcastChange();
}

bool UFaerieEquipmentSlot::CouldSetInSlot(const FFaerieItemStackView View) const
{
	if (!View.Item.IsValid()) return false;

	if (Config.SingleItemSlot && View.Copies > 1)
	{
		return false;
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions->AllowsAddition(this, MakeArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	if (IsValid(Config.SlotDescription))
	{
		return Config.SlotDescription->Template->TryMatch(View);
	}

	return false;
}

bool UFaerieEquipmentSlot::CanSetInSlot(const FFaerieItemStackView View) const
{
	if (!View.Item.IsValid()) return false;

	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (View.Item != ItemStack.Item)
		{
			return false;
		}

		if (Config.SingleItemSlot)
		{
			return false;
		}
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions->AllowsAddition(this, MakeArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	if (IsValid(Config.SlotDescription) &&
		IsValid(Config.SlotDescription->Template))
	{
		return Config.SlotDescription->Template->TryMatch(View);
	}

	return true;
}

bool UFaerieEquipmentSlot::CanTakeFromSlot(const int32 Copies) const
{
	if (!Faerie::ItemData::IsValidStack(Copies) ||
		!IsValid(ItemStack.Item)) return false;

	if (Copies != Faerie::ItemData::UnlimitedStack &&
		ItemStack.Copies < Copies)
	{
		return false;
	}

	if (Extensions->AllowsRemoval(this, GetCurrentAddress(), Faerie::Equipment::Tags::SlotTake) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieEquipmentSlot::SetItemInSlot(const FFaerieItemStack Stack)
{
	if (!CanSetInSlot(Stack))
	{
		UE_LOG(LogFaerieEquipment, Warning,
			TEXT("Invalid request to set into slot '%s'!"), *GetSlotInfo().ObjectName.ToString())
		return false;
	}

	// If the above check passes, then either the Stack's item is the same as our's, or we are currently empty!
	SetItemInSlot_Impl(Stack);
	return true;
}

FFaerieItemStack UFaerieEquipmentSlot::TakeItemFromSlot(int32 Copies)
{
	if (!CanTakeFromSlot(Copies))
	{
		UE_LOG(LogFaerieEquipment, Warning,
			TEXT("Invalid request to take item from slot '%s'!"), *GetSlotInfo().ObjectName.ToString())
		return FFaerieItemStack();
	}

	if (Copies > ItemStack.Copies)
	{
		UE_LOG(LogFaerieEquipment, Error,
			TEXT("Cannot remove more copies from a slot than what it contains. Slot: '%s', Requested Copies: '%i', Contained: '%i' !"),
			*GetSlotInfo().ObjectName.ToString(), Copies, ItemStack.Copies)
		return FFaerieItemStack();
	}

	if (Copies == Faerie::ItemData::UnlimitedStack)
	{
		Copies = ItemStack.Copies;
	}

	Extensions->PreRemoval(this, StoredKey, Copies);

	Faerie::Inventory::FEventLog Event;
	Event.Item = ItemStack.Item;
	Event.Amount = Copies;
	Event.Success = true;
	Event.Type = Faerie::Equipment::Tags::SlotTake;
	Event.EntryTouched = StoredKey;

	const FFaerieItemStack OutStack{ItemStack.Item, Copies};

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStack, this);
	if (Copies == ItemStack.Copies)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
		StoredKey = FEntryKey::InvalidKey;

		// Our local Item ptr must be nullptr before calling ReleaseOwnership
		ItemStack = FFaerieItemStack();

		// Release ownership of this item.
		Faerie::ReleaseOwnership(this, OutStack.Item);
	}
	else
	{
		ItemStack.Copies -= Copies;
	}

	Extensions->PostRemoval(this, Event);

	BroadcastChange();

	return OutStack;
}

FEntryKey UFaerieEquipmentSlot::GetCurrentKey() const
{
	return StoredKey;
}

FFaerieAddress UFaerieEquipmentSlot::GetCurrentAddress() const
{
	return FFaerieAddress(StoredKey.Value());
}

FFaerieAssetInfo UFaerieEquipmentSlot::GetSlotInfo() const
{
	if (IsValid(Config.SlotDescription) &&
		IsValid(Config.SlotDescription->Template))
	{
		return Config.SlotDescription->Template->GetDescription();
	}
	return FFaerieAssetInfo();
}

bool UFaerieEquipmentSlot::IsFilled() const
{
	return IsValid(ItemStack.Item) && ItemStack.Copies > 0;
}

UFaerieEquipmentSlot* UFaerieEquipmentSlot::FindSlot(const FFaerieSlotTag SlotTag, const bool bRecursive) const
{
	if (IsFilled())
	{
		const TSet<UFaerieEquipmentSlot*> Children = UFaerieItemContainerToken::GetContainersInItem<UFaerieEquipmentSlot>(ItemStack.Item);

		for (auto&& Child : Children)
		{
			if (Child->Config.SlotID == SlotTag)
			{
				return Child;
			}
		}

		if (bRecursive)
		{
			for (auto&& Child : Children)
			{
				if (auto&& ChildSlot = Child->FindSlot(Config.SlotID, true))
				{
					return ChildSlot;
				}
			}
		}
	}
	return nullptr;
}

void UFaerieEquipmentSlot::OnRep_Item()
{
	BroadcastChange();
}