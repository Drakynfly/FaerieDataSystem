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

		FDateTime Timestamp;

		TWeakObjectPtr<UFaerieItemContainerBase> Container;

		// The item from the modified entry.
		FFaerieItemInstance Instance;

		// Either the Addition tag, some kind of Removal, or an edit tag.
		FFaerieInventoryTag Type;

		// The number of copies added or removed. May be left as -1 on certain client-side events where the number of copies is unknown.
		int32 Copies = -1;

		// The entry that this event pertained to.
		FFaerieEntryKey EntryTouched;

		// All addresses that were modified by this event.
		TArray<FFaerieAddress> AddressesTouched;

		bool EntryRemoved = false;

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