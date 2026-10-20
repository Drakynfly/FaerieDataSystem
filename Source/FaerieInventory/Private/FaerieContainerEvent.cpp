// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerEvent.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemContainerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerEvent)

namespace Faerie::Container
{
	FContainerEventPayload FContainerEventPayload::MakeNew(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieEntryKey Entry, const FFaerieInventoryTag Type)
	{
		FContainerEventPayload NewEvent;
		NewEvent.Timestamp = FDateTime::UtcNow();
		NewEvent.Container = Container;
		NewEvent.Type = Type;
		NewEvent.EntryTouched = Entry;
		return NewEvent;
	}

	FContainerEventPayload FContainerEventPayload::MakeNewStack(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieItemInstance& ItemInstance, const int32 InCopies, const FFaerieEntryKey Entry,
		const TConstArrayView<FFaerieAddress> Addresses)
	{
		FContainerEventPayload NewEvent;
		NewEvent.Timestamp = FDateTime::UtcNow();
		NewEvent.Container = Container;
		NewEvent.Instance = ItemInstance;
		NewEvent.Copies = InCopies;
		NewEvent.Type = Inventory::Tags::NewStack;
		NewEvent.EntryTouched = Entry;
		NewEvent.AddressesTouched = Addresses;
		return NewEvent;
	}

	FContainerEventPayload FContainerEventPayload::MakeAddedCopies(const TNotNull<UFaerieItemContainerBase*> Container,
		const FFaerieItemInstance& ItemInstance, const int32 InCopies, const FFaerieEntryKey Entry,
		const TConstArrayView<FFaerieAddress> Addresses)
	{
		FContainerEventPayload NewEvent;
		NewEvent.Timestamp = FDateTime::UtcNow();
		NewEvent.Container = Container;
		NewEvent.Instance = ItemInstance;
		NewEvent.Copies = InCopies;
		NewEvent.Type = Inventory::Tags::AddedCopies;
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

	bool FContainerEventPayload::IsCopiesChangeEvent() const
	{
		return Type == Inventory::Tags::AddedCopies || Type == Inventory::Tags::RemovedCopies;
	}

	bool FContainerEventPayload::IsNewStackEvent() const
	{
		return Type == Inventory::Tags::NewStack;
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

	bool FContainerEventPayload::DebugCheck() const
	{
#if FAERIE_DEBUG
		bool HasError = false;

		if (Container.IsExplicitlyNull())
		{
			UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Container")
			HasError |= true;
		}
		if (!Type.IsValid())
		{
			UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Type")
			HasError |= true;
		}
		if (Instance.IsEmpty())
		{
			UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Instance")
			HasError |= true;
		}
		if (!ItemData::IsValidStackAmount(Copies))
		{
			UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Copies")
			HasError |= true;
		}
		if (!EntryTouched.IsValid())
		{
			UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Entry")
			HasError |= true;
		}
		for (auto It = AddressesTouched.CreateConstIterator(); It; ++It)
		{
			if (!It->IsValid())
			{
				UE_LOGF(LogFaerieInventory, Warning, "ContainerEventPayload::DebugCheck: Invalid Address[%d]", It.GetIndex())
				HasError |= true;
			}
		}
		return !HasError;
#else
		return true;
#endif
	}
}
