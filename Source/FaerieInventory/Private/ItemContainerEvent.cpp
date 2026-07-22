// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerEvent)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, Addition,
		"Fae.Inventory.Addition", "Inventory item data added event")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalBase,
		"Fae.Inventory.Removal", "Inventory item data removed event")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalDeletion,
		"Fae.Inventory.Removal.Deletion", "Remove an item by deleting it entirely")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalMoving,
		"Fae.Inventory.Removal.Moving", "Remove an item for the purpose of moving it elsewhere")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, EditBase,
		"Fae.Inventory.Edit", "Inventory item data changed event")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, ReplicationEdit,
		"Fae.Inventory.Edit.Replication", "Inventory item data changed by replication")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, Merge,
		"Fae.Inventory.Edit.Merge", "An entry was edited to merge two stacks")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, Split,
		"Fae.Inventory.Edit.Split", "An entry was edited to split an amount off onto a new stack")

	const TSet<FFaerieInventoryTag>& EditTagsAllowedByDefault()
	{
		static const TSet<FFaerieInventoryTag> StaticTags
		{
			Merge,
			Split
		};

		return StaticTags;
	}

	const TSet<FFaerieInventoryTag>& RemovalTagsAllowedByDefault()
	{
		static const TSet<FFaerieInventoryTag> StaticTags
		{
			RemovalDeletion,
			RemovalMoving
		};

		return StaticTags;
	}
}

FFaerieBlueprintInventoryEvent FFaerieBlueprintInventoryEvent::FromNativeEvent(const TNotNull<const UFaerieItemContainerBase*>& Container,
	const FFaerieInventoryTag Type, const Faerie::Inventory::FEventData& Data, const FDateTime Timestamp)
{
	return FFaerieBlueprintInventoryEvent(
		Timestamp,
		Container,
		Type,
		Data.Instance,
		Data.Copies,
		Data.EntryTouched,
		Data.AddressesTouched);
}

FFaerieBlueprintInventoryEvent FFaerieBlueprintInventoryEvent::FromNativeEvent(const TNotNull<const UFaerieItemContainerBase*>& Container,
	const Faerie::Inventory::FEventLogSingle& Event)
{
	return FFaerieBlueprintInventoryEvent(
		Event.GetTimestamp(),
		Container,
		Event.Type,
		Event.Data.Instance,
		Event.Data.Copies,
		Event.Data.EntryTouched,
		Event.Data.AddressesTouched);
}
