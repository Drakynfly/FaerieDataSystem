// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentSlot.h"

#include "FaerieEquipmentSlotDescription.h"
#include "FaerieAssetInfo.h"
#include "FaerieEquipmentLog.h"
#include "FaerieItem.h"
#include "FaerieItemStorageStatics.h"
#include "FaerieItemTemplate.h"
#include "FaerieSubObjectFilter.h"
#include "InventoryDataEnums.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentSlot)

using namespace Faerie;

void UFaerieEquipmentSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Config only needs to replicate once
	DOREPLIFETIME_CONDITION(ThisClass, Config, COND_InitialOnly);
}

//~ UFaerieItemContainerBase
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
		TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}

	// Cannot change Config here, as it only replicates once!

	if (SlotData.StoredKey.IsValid())
	{
		KeyGen.SetPosition(SlotData.StoredKey);

		const FFaerieItemStack& LoadedItemStack = SlotData.ItemStack;
		if (ItemData::ValidateItemData(LoadedItemStack.Item) &&
			LoadedItemStack.Copies > 0)
		{
			// We do need to ClearOwnership here, as whatever loaded the data may have parented the items automatically.
			if (auto Mutable = LoadedItemStack.Item->MutateCast())
			{
				ItemData::ClearOwnership(Mutable);
			}
			SetStoredItem_Impl(LoadedItemStack);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOG(LogFaerieEquipment, Error, TEXT("Loading content for slot '%s' failed. Slot has been emptied!"), *Config.SlotID.ToString())
			MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieItemStackContainer, StoredKey, this);
			StoredKey = FEntryKey();
		}
	}

	UnravelExtensionData(ExtensionData);
}
//~ UFaerieItemContainerBase

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

FFaerieAssetInfo UFaerieEquipmentSlot::GetSlotInfo() const
{
	if (IsValid(Config.SlotDescription) &&
		IsValid(Config.SlotDescription->Template))
	{
		return Config.SlotDescription->Template->GetDescription();
	}
	return FFaerieAssetInfo();
}

const UFaerieEquipmentSlot* UFaerieEquipmentSlot::FindSlot(const FFaerieSlotTag SlotTag, const bool bRecursive) const
{
	if (IsFilled())
	{
		auto Mutable = ItemStack.Item->MutateCast();
		if (!Mutable)
		{
			return nullptr;
		}

		const TArray<UFaerieEquipmentSlot*> Children = SubObject::Filter().ByClass<UFaerieEquipmentSlot>().Emit(Mutable);

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