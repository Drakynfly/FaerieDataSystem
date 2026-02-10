// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Libraries/LoggedInventoryEventLibrary.h"
#include "ItemContainerEvent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LoggedInventoryEventLibrary)

void ULoggedInventoryEventLibrary::BreakLoggedInventoryEvent(const FLoggedInventoryEvent& LoggedEvent, FFaerieInventoryTag& Type,
															 bool& Success, FDateTime& Timestamp, FEntryKey& EntryTouched,
															 TArray<FFaerieAddress>& AddressesTouched,
															 FFaerieItemStackView& Stack, FString& ErrorMessage)
{
	Type = LoggedEvent.Event.Type;
	Success = LoggedEvent.Event.Success;
	Timestamp = LoggedEvent.Event.GetTimestamp();
	EntryTouched = LoggedEvent.Event.EntryTouched;
	AddressesTouched = LoggedEvent.Event.AddressesTouched;
	Stack.Copies = LoggedEvent.Event.Amount;
	Stack.Item = LoggedEvent.Event.Item.IsValid() ? LoggedEvent.Event.Item.Get() : nullptr;
	ErrorMessage = LoggedEvent.Event.ErrorMessage;
}