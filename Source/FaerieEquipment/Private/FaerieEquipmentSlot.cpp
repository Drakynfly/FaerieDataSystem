// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentSlot.h"
#include "EntityManagerHelpers.h"

#include "FaerieEquipmentSlotDescription.h"
#include "FaerieEquipmentLog.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"
#include "FaerieItemOwnership.h"
#include "FaerieItemTemplate.h"
#include "FaerieSubObjectFilter.h"
#include "FaerieStorageEnums.h"
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
FInstancedStruct UFaerieEquipmentSlot::MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const
{
	return FInstancedStruct::Make(MakeSlotData(ExtensionData));
}

void UFaerieEquipmentSlot::LoadSaveData(const FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	const FFaerieEquipmentSlotSaveData* SlotSaveData = ItemData.GetPtr<const FFaerieEquipmentSlotSaveData>();
	if (!SlotSaveData)
	{
		return;
	}

	LoadSlotData(*SlotSaveData, ExtensionData);
}

FFaerieEquipmentSlotSaveData UFaerieEquipmentSlot::MakeSlotData(FFaerieItemContainerExtensionData& ExtensionData) const
{
	RavelExtensionData(ExtensionData);

	FFaerieEquipmentSlotSaveData SlotSaveData;
	SlotSaveData.SlotID = Config.SlotID;
	if (StoredKey.IsValid())
	{
		SlotSaveData.ItemObject = ItemStack.Instance.GetItemPtr();
		SlotSaveData.Copies = ItemStack.Copies;
		SlotSaveData.ExportData = ExportItemData(ItemData::FRequireEntityManager(this), ItemStack.Instance);
	}
	return SlotSaveData;
}

void UFaerieEquipmentSlot::LoadSlotData(const FFaerieEquipmentSlotSaveData& SlotData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData)
{
	if (!ensure(Config.SlotID == SlotData.SlotID))
	{
		return;
	}

	// Cannot change Config here, as it only replicates once!

	// Clear any current content.
	if (IsFilled())
	{
		TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}

	if (SlotData.Copies > 0)
	{
		// Rebuild instance from save data
		ItemData::FRequireEntityManager EntityManager(this);
		const FFaerieItemInstance Instance = ImportItemData(EntityManager, SlotData.ItemObject, SlotData.ExportData);

		if (Container::ValidateItemData(Instance) &&
			SlotData.Copies > 0)
		{
			// If it validated, store in slot.
			const FFaerieItemDataView DataView(Instance, SlotData.Copies, nullptr);
			SetStoredItem_Impl(DataView);
		}
		else
		{
			// Reset key if stack is invalid.
			UE_LOG(LogFaerieEquipment, Error, TEXT("Loading content for slot '%s' failed. Slot has been emptied!"), *Config.SlotID.ToString())
			MARK_PROPERTY_DIRTY_FROM_NAME(UFaerieItemStackContainer, StoredKey, this);
			StoredKey = FFaerieEntryKey();
		}
	}

	UnravelExtensionData(ExtensionData);
}
//~ UFaerieItemContainerBase

bool UFaerieEquipmentSlot::CouldSetInSlot(const FFaerieItemDataView& View) const
{
	if (!View.IsValid()) return false;

	const int32 ViewCopies = View.GetCopies();
	if (Config.SingleItemSlot && ViewCopies > 1)
	{
		return false;
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions::FGroupAPI::AllowsAddition(Extensions, this, MakeConstArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	if (IsValid(Config.SlotDescription))
	{
		return Config.SlotDescription->Template->TryMatch(this, View);
	}

	return false;
}

bool UFaerieEquipmentSlot::CanSetInSlot(const FFaerieItemDataView& View) const
{
	if (!View.IsValid()) return false;

	if (IsFilled())
	{
		// Cannot switch items. Remove current first.
		if (View.GetInstance() != ItemStack.Instance)
		{
			return false;
		}

		if (Config.SingleItemSlot)
		{
			return false;
		}
	}

	static constexpr FFaerieExtensionAllowsAdditionArgs Args = { EFaerieStorageAddStackBehavior::OnlyNewStacks };

	if (Extensions::FGroupAPI::AllowsAddition(Extensions, this, MakeConstArrayView(&View, 1), Args) == EEventExtensionResponse::Disallowed)
	{
		return false;
	}

	if (IsValid(Config.SlotDescription) &&
		IsValid(Config.SlotDescription->Template))
	{
		return Config.SlotDescription->Template->TryMatch(this, View);
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

const UFaerieEquipmentSlot* UFaerieEquipmentSlot::FindSlot(const ItemData::FRequireEntityManager& EntityManager, const FFaerieSlotTag SlotTag, const bool bRecursive) const
{
	if (IsFilled())
	{
		if (!ItemStack.Instance.IsMutable())
		{
			return nullptr;
		}

		const TArray<UFaerieEquipmentSlot*> Children = SubObject::Filter().ByClass<UFaerieEquipmentSlot>().Emit(EntityManager, ItemStack.Instance);

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
				if (auto&& ChildSlot = Child->FindSlot(EntityManager, Config.SlotID, true))
				{
					return ChildSlot;
				}
			}
		}
	}
	return nullptr;
}