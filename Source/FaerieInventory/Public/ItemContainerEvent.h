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
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Merge)
		FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, Split)

		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& EditTagsAllowedByDefault();
		FAERIEINVENTORY_API const TSet<FFaerieInventoryTag>& RemovalTagsAllowedByDefault();
	}

	// Logs that record data about additions to and removals from an item container.
	class FAERIEINVENTORY_API FEventLog
	{
	public:
		FEventLog()
		  : EventID(FGuid::NewGuid()),
			Timestamp(FDateTime::UtcNow()) {}

	private:
		static FEventLog CreateFailureEvent_Internal(const FFaerieInventoryTag Type, const FString& Message)
		{
			FEventLog NewErrorEvent;
			NewErrorEvent.Type = Type;
			NewErrorEvent.Success = false;
			NewErrorEvent.ErrorMessage = Message;
			return NewErrorEvent;
		}

	public:
		static FEventLog AdditionFailed(const FString& Message)
		{
			return CreateFailureEvent_Internal(Tags::Addition, Message);
		}

		const FGuid& GetEventID() const { return EventID; }
		const FDateTime& GetTimestamp() const { return Timestamp; }

		friend bool operator==(const FEventLog& Lhs, const FEventLog& Rhs)
		{
			return Lhs.EventID == Rhs.EventID;
		}

		friend bool operator!=(const FEventLog& Lhs, const FEventLog& Rhs)
		{
			return !(Lhs == Rhs);
		}

	public:
		// Either the Addition tag or some kind of Removal.
		FFaerieInventoryTag Type;

		// Did this event succeed?
		bool Success = false;

		// The entry that this event pertained to.
		FEntryKey EntryTouched;

		// All stacks that were modified by this event.
		TArray<FStackKey> StackKeys;

		// The number of item copies added or removed.
		int32 Amount = 0;

		// The item from this entry.
		TWeakObjectPtr<const UFaerieItem> Item;

		// Message, in case of a failure event.
		FString ErrorMessage;

		friend FArchive& operator<<(FArchive& Ar, FEventLog& Val)
		{
			return Ar << Val.Type
					  << Val.Success
					  << Val.EntryTouched
					  << Val.StackKeys
					  << Val.Amount
					  << Val.Item
					  << Val.ErrorMessage
					  << Val.EventID
					  << Val.Timestamp;
		}

	private:
		FGuid EventID;
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
	Faerie::Inventory::FEventLog Event;

	friend bool operator==(const FLoggedInventoryEvent& Lhs, const FLoggedInventoryEvent& Rhs)
	{
		return Lhs.Container == Rhs.Container && Lhs.Event == Rhs.Event;
	}

	friend bool operator!=(const FLoggedInventoryEvent& Lhs, const FLoggedInventoryEvent& Rhs)
	{
		return !(Lhs == Rhs);
	}

	friend FArchive& operator<<(FArchive& Ar, FLoggedInventoryEvent& Val)
	{
		Ar << Val.Container;
		Ar << Val.Event;
		return Ar;
	}
};