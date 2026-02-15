// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Libraries/LoggedInventoryEventLibrary.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LoggedInventoryEventLibrary)

void ULoggedInventoryEventLibrary::BreakLoggedInventoryEvent(const FLoggedInventoryEvent& LoggedEvent, FFaerieInventoryTag& Type,
															 FDateTime& Timestamp, FEntryKey& EntryTouched,
															 TArray<FFaerieAddress>& AddressesTouched,
															 FFaerieItemStackView& Stack)
{
	Type = LoggedEvent.Event.Type;
	Timestamp = LoggedEvent.Event.GetTimestamp();
	EntryTouched = LoggedEvent.Event.Data.EntryTouched;
	AddressesTouched = LoggedEvent.Event.Data.AddressesTouched;
	Stack.Copies = LoggedEvent.Event.Data.Amount;
	Stack.Item = LoggedEvent.Event.Data.Item.IsValid() ? LoggedEvent.Event.Data.Item.Get() : nullptr;
}