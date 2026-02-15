// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "GameplayTagContainer.h"
#include "TypedGameplayTags.h"
#include "InventoryDataStructs.h"
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
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Merge)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Split)

		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& EditTagsAllowedByDefault();
		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& RemovalTagsAllowedByDefault();
	}

	class FAERIEINVENTORY_API FEventData
	{
	public:
		FEventData() {}

	public:
		// The item from this entry.
		TWeakObjectPtr<const UFaerieItem> Item;

		// The number of item copies added or removed.
		int32 Amount = 0;

		// The entry that this event pertained to.
		FEntryKey EntryTouched;

		// All addresses that were modified by this event.
		TArray<FFaerieAddress> AddressesTouched;

		friend FArchive& operator<<(FArchive& Ar, FEventData& Val)
		{
			return Ar << Val.EntryTouched
					  << Val.AddressesTouched
					  << Val.Amount
					  << Val.Item;
		}
	};

	// Logs that record data about additions to and removals from an item container.
	class FAERIEINVENTORY_API FEventLogSingle
	{
	public:
		FEventLogSingle() = default;

		FEventLogSingle(const FFaerieInventoryTag Type, const FEventData& Data)
		  : Type(Type),
			Data(Data),
			Timestamp(FDateTime::UtcNow())
		{}

		bool IsAdditionEvent() const { return Type == Tags::Addition; }
		bool IsRemovalEvent() const { return Type.MatchesTag(Tags::RemovalBase); }
		bool IsEditEvent() const { return Type.MatchesTag(Tags::EditBase); }

		const FDateTime& GetTimestamp() const { return Timestamp; }

	public:
		// Either the Addition tag, some kind of Removal, or an edit tag.
		FFaerieInventoryTag Type;

		FEventData Data;

		friend FArchive& operator<<(FArchive& Ar, FEventLogSingle& Val)
		{
			return Ar << Val.Type
					  << Val.Data
					  << Val.Timestamp;
		}

	private:
		FDateTime Timestamp;
	};

	// A group of events that occured at once.
	class FAERIEINVENTORY_API FEventLogBatch
	{
	public:
		FEventLogBatch()
		  : Timestamp(FDateTime::UtcNow()) {}

		const FDateTime& GetTimestamp() const { return Timestamp; }

		bool IsAdditionEvent() const { return Type == Tags::Addition; }
		bool IsRemovalEvent() const { return Type.MatchesTag(Tags::RemovalBase); }
		bool IsEditEvent() const { return Type.MatchesTag(Tags::EditBase); }

		// Either the Addition tag, some kind of Removal, or an edit tag.
		FFaerieInventoryTag Type;

	public:
		TConstArrayView<FEventData> Data;

	private:
		FDateTime Timestamp;
	};
}

/*
 * Blueprint wrapper of Faerie::Inventory::FEventLog
 */
USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/FaerieInventory.LoggedInventoryEventLibrary.BreakLoggedInventoryEvent"))
struct FAERIEINVENTORY_API FLoggedInventoryEvent
{
	GENERATED_BODY()

	// Which storage logged this event
	UPROPERTY()
	TWeakObjectPtr<const class UFaerieItemContainerBase> Container = nullptr;

	// The logged event
	Faerie::Inventory::FEventLogSingle Event;

	friend FArchive& operator<<(FArchive& Ar, FLoggedInventoryEvent& Val)
	{
		Ar << Val.Container;
		Ar << Val.Event;
		return Ar;
	}
};