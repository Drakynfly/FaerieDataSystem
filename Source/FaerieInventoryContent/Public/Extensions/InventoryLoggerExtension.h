﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "ItemContainerEvent.h"
#include "InventoryLoggerExtension.generated.h"

using FInventoryEventLoggedNative = TMulticastDelegate<void(const FLoggedInventoryEvent&)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryEventLogged, const FLoggedInventoryEvent&, LoggedEvent);

/**
 * Logs events from additions, changes, and removals, and can parse them for data at request.
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UInventoryLoggerExtension : public UItemContainerExtensionBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostRemoval(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;

	void HandleNewEvent(const FLoggedInventoryEvent& Event);

public:
	FInventoryEventLoggedNative::RegistrationType& GetOnInventoryEventLogged() { return OnInventoryEventLoggedNative; }

	UFUNCTION(BlueprintCallable, Category = "LoggerExtension")
	int32 GetNumEvents() const { return EventLog.Num(); }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "LoggerExtension")
	const TArray<FLoggedInventoryEvent>& GetAllEvents() const { return EventLog; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "LoggerExtension")
	TArray<FLoggedInventoryEvent> GetRecentEvents(int32 NumEvents) const;

protected:
	UFUNCTION(/* Replication */)
	virtual void OnRep_EventLog();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FInventoryEventLogged OnInventoryEventLogged;

	UPROPERTY(ReplicatedUsing = "OnRep_EventLog")
	TArray<FLoggedInventoryEvent> EventLog;

private:
	FInventoryEventLoggedNative OnInventoryEventLoggedNative;

	int32 LocalEventLogCount = 0;
};