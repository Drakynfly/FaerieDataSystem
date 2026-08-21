// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "GameplayTagContainer.h"
#include "TypedGameplayTags.h"
#include "FaerieStorageStructs.h"
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

	class FAERIEINVENTORY_API FEventData
	{
	public:
		//UE_NONCOPYABLE(FEventData)

		FEventData(const FFaerieEntryKey Entry)
		  : EntryTouched(Entry) {}

		FEventData(const FFaerieItemInstance& Instance, const int32 Copies, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses)
		  : Instance(Instance), Copies(Copies), EntryTouched(Entry), AddressesTouched(Addresses) {}

		// The item from the modified entry.
		FFaerieItemInstance Instance;

		// The number of copies added or removed. May be left as -1 on certain client-side events where the number of copies is unknown.
		int32 Copies = -1;

		// The entry that this event pertained to.
		const FFaerieEntryKey EntryTouched;

		// All addresses that were modified by this event.
		TArray<FFaerieAddress> AddressesTouched;

		bool EntryRemoved = false;
	};

	// Logs that record data about additions to and removals from an item container.
	class FAERIEINVENTORY_API FEventLogSingle
	{
	public:
		UE_NONCOPYABLE(FEventLogSingle)

		FEventLogSingle(const FFaerieInventoryTag Type, const FFaerieItemInstance& Instance, const int32 Copies, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses)
		  : Type(Type),
			Data(Instance, Copies, Entry, Addresses),
			Timestamp(FDateTime::UtcNow())
		{}

		bool IsAdditionEvent() const { return Type == Tags::Addition; }
		bool IsRemovalEvent() const { return Type.MatchesTag(Tags::RemovalBase); }
		bool IsEditEvent() const { return Type.MatchesTag(Tags::EditBase); }

		const FDateTime& GetTimestamp() const { return Timestamp; }

	public:
		// Either the Addition tag, some kind of Removal, or an edit tag.
		const FFaerieInventoryTag Type;

		const FEventData Data;

	private:
		const FDateTime Timestamp;
	};

	// A group of events that occured at once.
	class FAERIEINVENTORY_API FEventLogBatch
	{
	public:
		UE_NONCOPYABLE(FEventLogBatch)

		FEventLogBatch(const TConstArrayView<FEventData> EventArrayRef, const FFaerieInventoryTag Type)
		  : Type(Type), Data(EventArrayRef), Timestamp(FDateTime::UtcNow()) {}

		const FDateTime& GetTimestamp() const { return Timestamp; }

		bool IsAdditionEvent() const { return Type == Tags::Addition; }
		bool IsRemovalEvent() const { return Type.MatchesTag(Tags::RemovalBase); }
		bool IsEditEvent() const { return Type.MatchesTag(Tags::EditBase); }
		bool IsReplicationEvent() const { return Type == Tags::ReplicationEdit; }

		const FFaerieInventoryTag Type;

	public:
		const TConstArrayView<FEventData> Data;

	private:
		const FDateTime Timestamp;
	};
}

/*
 * Blueprint wrapper of Faerie::Inventory::FEventLogSingle
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieBlueprintInventoryEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FDateTime Timestamp;

	// Which storage logged this event
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	TWeakObjectPtr<const class UFaerieItemContainerBase> Container = nullptr;

	// Either the Addition tag, some kind of Removal, or an edit tag.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FFaerieInventoryTag Type;

	// The item from this entry.
	// @todo not exposed to Blueprint
	UPROPERTY()
	FFaerieItemInstance Item;

	// The number of item copies added or removed.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	int32 Copies = 0;

	// The entry that this event pertained to.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	FFaerieEntryKey EntryTouched;

	// All addresses that were modified by this event.
	UPROPERTY(BlueprintReadOnly, Category = "InventoryEvent")
	TArray<FFaerieAddress> AddressesTouched;

	static FFaerieBlueprintInventoryEvent FromNativeEvent(const TNotNull<const UFaerieItemContainerBase*>& Container, FFaerieInventoryTag Type, const Faerie::Inventory::FEventData& Data, FDateTime Timestamp);
	static FFaerieBlueprintInventoryEvent FromNativeEvent(const TNotNull<const UFaerieItemContainerBase*>& Container, const Faerie::Inventory::FEventLogSingle& Event);
};