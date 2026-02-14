// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Extensions/InventoryLoggerExtension.h"
#include "FaerieItemContainerBase.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryLoggerExtension)

void UInventoryLoggerExtension::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, EventLog, SharedParams)
}

void UInventoryLoggerExtension::PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	HandleNewEvent({Container, Event});
}

void UInventoryLoggerExtension::PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	HandleNewEvent({Container, Event});
}

void UInventoryLoggerExtension::PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event)
{
	HandleNewEvent({Container, Event});
}

void UInventoryLoggerExtension::HandleNewEvent(const FLoggedInventoryEvent& Event)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EventLog, this);
	EventLog.Add(Event);
	OnInventoryEventLoggedNative.Broadcast(Event);
	OnInventoryEventLogged.Broadcast(Event);
}

TArray<FLoggedInventoryEvent> UInventoryLoggerExtension::GetRecentEvents(const int32 NumEvents, const int32 Offset) const
{
	return TArray<FLoggedInventoryEvent>(MakeConstArrayView(EventLog).Mid(EventLog.Num() - Offset - NumEvents, NumEvents));
}

void UInventoryLoggerExtension::OnRep_EventLog()
{
	// When the EventLog is replicated to clients, we need to check how many events behind we are.
	const int32 BehindCount = EventLog.Num() - LocalEventLogCount;
	LocalEventLogCount = EventLog.Num();

	for (auto&& RecentLogs = MakeConstArrayView(EventLog).Right(BehindCount);
		auto&& RecentLog : RecentLogs)
	{
		OnInventoryEventLogged.Broadcast(RecentLog);
	}
}