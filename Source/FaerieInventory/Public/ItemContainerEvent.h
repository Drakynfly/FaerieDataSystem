// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "FaerieItemContainerStructs.h"
#include "TypedGameplayTags.h"
#include "ItemContainerEvent.generated.h"

namespace Faerie::Inventory
{
	namespace Tags
	{
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Addition)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalBase)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalDeletion)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalMoving)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, EditBase)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, ReplicationEdit)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Merge)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Split)

		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& EditTagsAllowedByDefault();
		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& RemovalTagsAllowedByDefault();
	}
 }

namespace Faerie::Container
{
	struct FEvent;
}

/*
 * Blueprint wrapper of Faerie::Container::FEvent. The data is the same, but I keep it a separate type for flexibility.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieBlueprintInventoryEvent
{
	GENERATED_BODY()

	// Which storage logged this event
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	TWeakObjectPtr<const class UFaerieItemContainerBase> Container = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FDateTime Timestamp;

	// Either the Addition tag, some kind of Removal, or an edit tag.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FFaerieInventoryTag Type;

	// The number of item copies added or removed.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	int32 Copies = 0;

	// The entry that this event pertained to.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FFaerieEntryKey EntryTouched;

	// All addresses that were modified by this event.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	TArray<FFaerieAddress> AddressesTouched;

	static FFaerieBlueprintInventoryEvent FromNativeEvent(const Faerie::Container::FEvent& NativeEvent);
};