// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Fragments/FaerieItemStorageFragment.h"
#include "FaerieContainerEvent.generated.h"

#define FAE_API FAERIEINVENTORY_API

namespace Faerie::Container
{
	USTRUCT()
	struct FEvent : public FMassFragment
	{
		GENERATED_BODY()

		static FEvent MakeBlank(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieEntryKey Entry)
		{
			FEvent NewEvent;
			NewEvent.Timestamp = FDateTime::UtcNow();
			NewEvent.Container = Container;
			NewEvent.Type = Inventory::Tags::Addition;
			NewEvent.EntryTouched = Entry;
			return NewEvent;
		}

		static FEvent MakeAddition(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemInstance& ItemInstance,
			const int32 InCopies, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses)
		{
			FEvent NewEvent;
			NewEvent.Timestamp = FDateTime::UtcNow();
			NewEvent.Container = Container;
			NewEvent.Instance = ItemInstance;
			NewEvent.Copies = InCopies;
			NewEvent.Type = Inventory::Tags::Addition;
			NewEvent.EntryTouched = Entry;
			NewEvent.AddressesTouched = Addresses;
			return NewEvent;
		}

		static FEvent MakeRemoval(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemInstance& ItemInstance,
			const int32 InCopies, const FFaerieInventoryTag Reason, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses, const bool InEntryRemoved)
		{
			FEvent NewEvent;
			NewEvent.Timestamp = FDateTime::UtcNow();
			NewEvent.Container = Container;
			NewEvent.Instance = ItemInstance;
			NewEvent.Copies = InCopies;
			NewEvent.Type = Reason;
			NewEvent.EntryTouched = Entry;
			NewEvent.AddressesTouched = Addresses;
			NewEvent.EntryRemoved = InEntryRemoved;
			return NewEvent;
		}

		FDateTime Timestamp;

		TWeakObjectPtr<UFaerieItemContainerBase> Container;

		// The item from the modified entry.
		FFaerieItemInstance Instance;

		// The number of copies added or removed. May be left as -1 on certain client-side events where the number of copies is unknown.
		int32 Copies = -1;

		// Either the Addition tag, some kind of Removal, or an edit tag.
		FFaerieInventoryTag Type;

		// The entry that this event pertained to.
		FFaerieEntryKey EntryTouched;

		// For removal events, was the entry removed.
		// @todo can we bake this into the tag please
		bool EntryRemoved = false;

		// All addresses that were modified by this event.
		TArray<FFaerieAddress> AddressesTouched;

		FAE_API bool IsAdditionEvent() const;
		FAE_API bool IsRemovalEvent() const;
		FAE_API bool IsEditEvent() const;
		FAE_API bool IsReplicationEvent() const;
	};

	// Event data contains an array... can we do anything about this?
	template<>
	struct TMassFragmentTraits<FEvent> final
	{
		enum
		{
			AuthorAcceptsItsNotTriviallyCopyable = true
		};
	};

	const FName EventCleanup = TEXT("EventCleanup");
}

#undef FAE_API