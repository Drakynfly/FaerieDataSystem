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

void UInventoryLoggerExtension::PostEventBatch(const TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, EventLog, this);
	for (auto&& Event : Events.Data)
	{
		EventLog.Emplace(Container, Faerie::Inventory::FEventLogSingle(Events.Type, Event));
	}
	OnInventoryEventLoggedNative.Broadcast(Events.Data.Num());
	OnInventoryEventLogged.Broadcast(Events.Data.Num());
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
	OnInventoryEventLogged.Broadcast(BehindCount);
}