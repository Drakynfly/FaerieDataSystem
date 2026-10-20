// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "FaerieItemContainerStructs.h"
#include "TypedGameplayTags.h"
#include "ItemContainerEvent.generated.h"

#define FAE_API FAERIEINVENTORY_API

namespace Faerie::Inventory
{
	namespace Tags
	{
		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, NewStack)

		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalBase)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalDeletion)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovalMoving)

		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, AddedCopies)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, RemovedCopies)

		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, EditBase)
		FAE_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, ReplicationEdit)
	}
}

namespace Faerie::Container
{
	struct FContainerEventPayload;
}

/*
 * Blueprint wrapper of Faerie::Container::FContainerEventPayload. The data is the same, but I keep it a separate type for flexibility.
 */
USTRUCT(BlueprintType)
struct FAE_API FFaerieBlueprintInventoryEvent
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

	static FFaerieBlueprintInventoryEvent FromNativeEvent(const Faerie::Container::FContainerEventPayload& NativeEvent);
};

#undef FAE_API