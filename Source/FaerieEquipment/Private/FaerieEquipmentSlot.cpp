﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentSlot.h"

#include "FaerieEquipmentSlotDescription.h"
#include "FaerieAssetInfo.h"
#include "FaerieItem.h"
#include "FaerieItemDataStatics.h"
#include "FaerieItemTemplate.h"
#include "InventoryDataEnums.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"
#include "Tokens/FaerieChildSlotToken.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Providers/FlakesBinarySerializer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentSlot)

DEFINE_LOG_CATEGORY(LogFaerieEquipmentSlot)

namespace Faerie::Equipment::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotSet, "Fae.Inventory.Set", "Event tag when item data is added to a slot")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, SlotTake, "Fae.Inventory.Take", "Event tag when item data is removed from a slot")
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

FFaerieContainerSaveData UFaerieEquipmentSlot::MakeSaveData() const
{
	FFaerieEquipmentSlotSaveData SlotSaveData;
	SlotSaveData.Config = Config;
	SlotSaveData.StoredKey = StoredKey;
	if (StoredKey.IsValid())
	{
		SlotSaveData.ItemStack = Flakes::MakeFlake<Flakes::Binary::Type>(FConstStructView::Make(ItemStack), this);
	}

	FFaerieContainerSaveData SaveData;
	SaveData.ItemData = FInstancedStruct::Make(SlotSaveData);
	RavelExtensionData(SaveData.ExtensionData);
	return SaveData;
}

void UFaerieEquipmentSlot::LoadSaveData(const FFaerieContainerSaveData& SaveData)
{
	const FFaerieEquipmentSlotSaveData* SlotSaveData = SaveData.ItemData.GetPtr<FFaerieEquipmentSlotSaveData>();
	if (!SlotSaveData)
	{
		return;
	}

	// @todo should check all Config members, not just SlotID
	if (!ensure(Config.SlotID == SlotSaveData->Config.SlotID))
	{
		return;
	}

	// Clear any current content.
	if (IsFilled())
	{
		TakeItemFromSlot(-1);
	}

	// Cannot change Config here, as it only replicates once!

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StoredKey, this);
	StoredKey = SlotSaveData->StoredKey;

	if (StoredKey.IsValid())
	{
		KeyGen.SetPosition(StoredKey);

		const FFaerieItemStack LoadedItemStack = Flakes::CreateStruct<Flakes::Binary::Type, FFaerieItemStack>(SlotSaveData->ItemStack, this);
		if (Faerie::ValidateLoadedItem(LoadedItemStack.Item) &&
			LoadedItemStack.Copies > 0)
		{
			SetItemInSlot_Impl(LoadedItemStack);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOG(LogFaerieEquipmentSlot, Error, TEXT("Loading content for slot '%s' failed. Slot has been emptied!"), *Config.SlotID.ToString())
			StoredKey = FEntryKey();
		}
	}

	UnravelExtensionData(SaveData.ExtensionData);
}

//~ UFaerieItemContainerBase
bool UFaerieEquipmentSlot::Contains(const FEntryKey Key) const
{
	return StoredKey == Key;
}

void UFaerieEquipmentSlot::ForEachKey(const TFunctionRef<void(FEntryKey)>& Func) const
{
	if (StoredKey.IsValid())
	{
		Func(StoredKey);
	}
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

FFaerieItemProxy UFaerieEquipmentSlot::Proxy(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return this;
	}
	return nullptr;
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

TArray<FFaerieAddress> UFaerieEquipmentSlot::Switchover_GetAddresses(const FEntryKey Key) const
{
	if (Contains(Key))
	{
		return { GetCurrentAddress() };
	}
	return {};
}

void UFaerieEquipmentSlot::ForEachAddress(const TFunctionRef<void(FFaerieAddress)>& Func) const
{
	if (StoredKey.IsValid())
	{
		Func(GetCurrentAddress());
	}
}

void UFaerieEquipmentSlot::ForEachItem(const TFunctionRef<void(const UFaerieItem*)>& Func) const
{
	if (StoredKey.IsValid())
	{
		Func(ItemStack.Item);
	}
}

void UFaerieEquipmentSlot::OnItemMutated(const UFaerieItem* InItem, const UFaerieItemToken* Token, const FGameplayTag EditTag)
{
	Super::OnItemMutated(InItem, Token, EditTag);
	check(ItemStack.Item == InItem);

	BroadcastDataChange();
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
		TakeOwnership(ItemStack.Item);

		Event.EntryTouched = StoredKey;
		Extensions->PostAddition(this, Event);
	}
	else
	{
		ItemStack.Copies += Stack.Copies;

		Event.EntryTouched = StoredKey;
		Extensions->PostEntryChanged(this, Event);
	}

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

	if (Extensions->AllowsRemoval(this, StoredKey, Faerie::Equipment::Tags::SlotTake) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	return true;
}

bool UFaerieEquipmentSlot::SetItemInSlot(const FFaerieItemStack Stack)
{
	if (!CanSetInSlot(Stack))
	{
		UE_LOG(LogFaerieEquipmentSlot, Warning,
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
		UE_LOG(LogFaerieEquipmentSlot, Warning,
			TEXT("Invalid request to take item from slot '%s'!"), *GetSlotInfo().ObjectName.ToString())
		return FFaerieItemStack();
	}

	if (Copies > ItemStack.Copies)
	{
		UE_LOG(LogFaerieEquipmentSlot, Error,
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
		ReleaseOwnership(OutStack.Item);

		Extensions->PostRemoval(this, Event);
	}
	else
	{
		ItemStack.Copies -= Copies;
		Extensions->PostEntryChanged(this, Event);
	}

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