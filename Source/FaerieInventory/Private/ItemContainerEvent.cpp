// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemContainerEvent.h"
#include "FaerieContainerEvent.h"
#include "FaerieItemContainerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerEvent)

namespace Faerie::Inventory::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, NewStack,
		"Fae.Inventory.Addition", "The stored stack was changed, either the item, number of copies, or both.")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalBase,
		"Fae.Inventory.Removal", "Base item data removed event tag. Actual events should always use a child tag.")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalDeletion,
		"Fae.Inventory.Removal.Deletion", "Remove an item by deleting it entirely")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovalMoving,
		"Fae.Inventory.Removal.Moving", "Remove an item for the purpose of moving it elsewhere")

	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, AddedCopies,
		"Fae.Inventory.AddedCopies", "Stack copies increased event")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, RemovedCopies,
		"Fae.Inventory.RemovedCopies", "Stack copies decremented event")

	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, EditBase,
		"Fae.Inventory.Edit", "Item data changed event")
	UE_DEFINE_GAMEPLAY_TAG_TYPED_COMMENT(FFaerieInventoryTag, ReplicationEdit,
		"Fae.Inventory.Edit.Replication", "Inventory item data changed by replication")
}

FFaerieBlueprintInventoryEvent FFaerieBlueprintInventoryEvent::FromNativeEvent(const Faerie::Container::FContainerEventPayload& NativeEvent)
{
	return FFaerieBlueprintInventoryEvent{
		.Container = NativeEvent.Container,
		.Timestamp = NativeEvent.Timestamp,
		.Type = NativeEvent.Type,
		.Copies = NativeEvent.Copies,
		.EntryTouched = NativeEvent.EntryTouched,
		.AddressesTouched = NativeEvent.AddressesTouched
	};
}