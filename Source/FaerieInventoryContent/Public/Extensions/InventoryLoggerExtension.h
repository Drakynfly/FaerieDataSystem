// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerDataViewModelBase.h"
#include "ItemContainerExtensionBase.h"
#include "ItemContainerEvent.h"
#include "MassProcessor.h"
#include "InventoryLoggerExtension.generated.h"

class UFaerieContainerEventLogView;

/*
 * Caches event history of a container, allowing UI to display a list of recent changes made.
 */
USTRUCT()
struct FFaerieContainerEventLog : public FFaerieItemContainerData
{
	GENERATED_BODY()

	// @todo this is a trivial use-case, but it would be nice to use this as an example of figuring out how to use
	// delta replication / fast arrays on a non-UObject owned array, as this is a rather lorge array to replicate
	UPROPERTY()
	TArray<FFaerieBlueprintInventoryEvent> EventLog;

	UPROPERTY(EditAnywhere, Category = "ContainerEventLog")
	int32 MaxEventsToStore = 50;

____FAERIE_CONTAINER_DATA_DECL(FFaerieContainerEventLog)
};

UCLASS()
class UFaerieContainerEventLogView : public UFaerieContainerDataViewModelBase
{
	GENERATED_BODY()

public:
	//~ UFaerieContainerDataViewModelBase
	virtual void SyncView() override;
	//~ UFaerieContainerDataViewModelBase

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Faerie|ContainerEventLogView")
	bool AreEventsLogged() const;

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Faerie|ContainerEventLogView")
	int32 GetNumEvents() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	FFaerieBlueprintInventoryEvent GetEvent(int32 Index, bool FromEnd) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void PreviousPageIndex();

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void NextPageIndex();

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void SetPageIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void SetCountPerPage(int32 Count);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void SetFilterTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ContainerEventLogView")
	void SetInvertEventOrder(bool Invert);

protected:
	void RecalculateEventTags();
	void RecalculateLogView();

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ContainerEventLogView")
	TArray<FFaerieBlueprintInventoryEvent> PageView;

	// The current page index being viewed.
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetPageIndex, Category = "ContainerEventLogView")
	int32 PageIndex = 0;

	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetCountPerPage, Category = "ContainerEventLogView")
	int32 CountPerPage = 10;

	// Only show events with this tag.
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetFilterTags, Category = "ContainerEventLogView", meta = (Categories = "Fae.Inventory"))
	FGameplayTagContainer FilterTags;

	// Switch between most-recent order (true) and chronological order (false).
	UPROPERTY(BlueprintReadWrite, FieldNotify, BlueprintSetter = SetInvertEventOrder, Category = "ContainerEventLogView")
	bool InvertEventOrder = false;

	// The number of pages (with filter applied).
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "ContainerEventLogView")
	int32 NumPages = 0;

	// The total number of events captured (with filter applied).
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ContainerEventLogView")
	int32 NumFilteredEvents = 0;

	// All tags for all logged events.
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ContainerEventLogView")
	FGameplayTagContainer EventTags;
};

namespace Faerie::Content
{
	USTRUCT()
	struct FEventLogViewFragment : public Container::FViewModelFragment
	{
		GENERATED_BODY()
	};
}

UCLASS()
class UFaerieContainerEventLogUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerEventLogUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
	FMassEntityQuery ViewQuery;
};