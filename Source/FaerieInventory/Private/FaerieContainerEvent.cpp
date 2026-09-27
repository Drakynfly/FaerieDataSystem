// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"
#include "FaerieItemContainerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerEvent)

namespace Faerie::Container
{
	FContainerEventPayload FContainerEventPayload::MakeBlank(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieEntryKey Entry)
	{
		FContainerEventPayload NewEvent;
		NewEvent.Timestamp = FDateTime::UtcNow();
		NewEvent.Container = Container;
		NewEvent.Type = Inventory::Tags::Addition;
		NewEvent.EntryTouched = Entry;
		return NewEvent;
	}

	FContainerEventPayload FContainerEventPayload::MakeAddition(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieItemInstance& ItemInstance, const int32 InCopies, const FFaerieEntryKey Entry,
		const TConstArrayView<FFaerieAddress> Addresses)
	{
		FContainerEventPayload NewEvent;
		NewEvent.Timestamp = FDateTime::UtcNow();
		NewEvent.Container = Container;
		NewEvent.Instance = ItemInstance;
		NewEvent.Copies = InCopies;
		NewEvent.Type = Inventory::Tags::Addition;
		NewEvent.EntryTouched = Entry;
		NewEvent.AddressesTouched = Addresses;
		return NewEvent;
	}

	FContainerEventPayload FContainerEventPayload::MakeRemoval(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieItemInstance& ItemInstance, const int32 InCopies, const FFaerieInventoryTag Reason,
		const FFaerieEntryKey Entry, const TConstArrayView<FFaerieAddress> Addresses, const bool InEntryRemoved)
	{
		FContainerEventPayload NewEvent;
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

	bool FContainerEventPayload::IsAdditionEvent() const
	{
		return Type == Inventory::Tags::Addition;
	}

	bool FContainerEventPayload::IsRemovalEvent() const
	{
		return Type.MatchesTag(Inventory::Tags::RemovalBase);
	}

	bool FContainerEventPayload::IsEditEvent() const
	{
		return Type.MatchesTag(Inventory::Tags::EditBase);
	}

	bool FContainerEventPayload::IsReplicationEvent() const
	{
		return Type == Inventory::Tags::ReplicationEdit;
	}
}
