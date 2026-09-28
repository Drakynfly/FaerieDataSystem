// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieItemInstance.h"
#include "ItemContainerEvent.h"
#include "Mass/EntityElementTypes.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "FaerieContainerEvent.generated.h"

#define FAE_API FAERIEINVENTORY_API

class UFaerieItemContainerBase;

namespace Faerie::Container
{
	USTRUCT()
	struct FContainerEventPayload : public FMassFragment
	{
		GENERATED_BODY()

		FAE_API static FContainerEventPayload MakeBlank(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieEntryKey Entry);

		FAE_API static FContainerEventPayload MakeAddition(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemInstance& ItemInstance,
			const int32 InCopies, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses);

		FAE_API static FContainerEventPayload MakeRemoval(const TNotNull<UFaerieItemContainerBase*> Container, const FFaerieItemInstance& ItemInstance,
			const int32 InCopies, const FFaerieInventoryTag Reason, const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses, const bool InEntryRemoved);

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

	const FName EventCleanup = TEXT("ContainerEventCleanup");
}

// Event data contains an array... can we do anything about this?
template<>
struct TMassFragmentTraits<Faerie::Container::FContainerEventPayload> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

#undef FAE_API