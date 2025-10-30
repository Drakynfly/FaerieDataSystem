// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilter.h"
#include "FaerieFunctionTemplates.h"
#include "UObject/Object.h"
#include "FaerieItemStorageQuery.generated.h"

class UFaerieItemDataComparator;
class UFaerieItemDataFilter;
class UFaerieItemStorage;
class UFaerieItemStorageQuery;

// We need to expose these delegates to the global namespace or UHT will cry.
using FFaerieItemPredicate = UFaerieFunctionTemplates::FFaerieItemPredicate;
using FFaerieStackPredicate = UFaerieFunctionTemplates::FFaerieStackPredicate;
using FFaerieSnapshotPredicate = UFaerieFunctionTemplates::FFaerieSnapshotPredicate;
using FFaerieItemComparator = UFaerieFunctionTemplates::FFaerieItemComparator;
using FFaerieStackComparator = UFaerieFunctionTemplates::FFaerieStackComparator;
using FFaerieSnapshotComparator = UFaerieFunctionTemplates::FFaerieSnapshotComparator;

namespace Faerie
{
	using FItemStorageQueryEvent = TMulticastDelegate<void(const UFaerieItemStorageQuery*)>;
}

/**
 * A UObject wrapper around the Faerie::Storage API.
 */
UCLASS(BlueprintType)
class FAERIEINVENTORY_API UFaerieItemStorageQuery : public UObject
{
	GENERATED_BODY()

public:
	Faerie::FItemStorageQueryEvent::RegistrationType& GetQueryChangedEvent() { return OnQueryChanged; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool IsSortBound() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool IsFilterBound() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool GetInvertFilter() const { return InvertFilter; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool GetInvertSort() const { return InvertSort; }

	void SetFilter(Faerie::Container::FItemPredicate&& Predicate, UObject* AssociatedUObject);
	void SetFilter(Faerie::Container::FStackPredicate&& Predicate, UObject* AssociatedUObject);
	void SetFilter(Faerie::Container::FSnapshotPredicate&& Predicate, UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Filter by Item Delegate")
	void SetFilterByDelegate_Item(const UFaerieFunctionTemplates::FFaerieItemPredicate& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Filter by Stack Delegate")
	void SetFilterByDelegate_Stack(const UFaerieFunctionTemplates::FFaerieStackPredicate& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Filter by Snapshot Delegate")
	void SetFilterByDelegate_Snapshot(const UFaerieFunctionTemplates::FFaerieSnapshotPredicate& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetFilterByObject(const UFaerieItemDataFilter* Object);

	void SetSort(Faerie::Container::FItemComparator&& Comparator, UObject* AssociatedUObject);
	void SetSort(Faerie::Container::FStackComparator&& Comparator, UObject* AssociatedUObject);
	void SetSort(Faerie::Container::FSnapshotComparator&& Comparator, UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Sort by Item Delegate")
	void SetSortByDelegate_Item(const UFaerieFunctionTemplates::FFaerieItemComparator& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Sort by Stack Delegate")
	void SetSortByDelegate_Stack(const UFaerieFunctionTemplates::FFaerieStackComparator& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query", DisplayName = "Set Sort by Snapshot Delegate")
	void SetSortByDelegate_Snapshot(const UFaerieFunctionTemplates::FFaerieSnapshotComparator& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetSortByObject(const UFaerieItemDataComparator* Comparator);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetInvertFilter(bool Invert);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetInvertSort(bool Invert);

	// Query function to filter and sort for a subsection of contained entries.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	FFaerieAddress QueryFirstAddress(const UFaerieItemStorage* Storage) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void QueryAllAddresses(const UFaerieItemStorage* Storage, TArray<FFaerieAddress>& OutAddresses) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool CompareAddresses(const UFaerieItemStorage* Storage, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool IsAddressFiltered(const UFaerieItemStorage* Storage, const FFaerieAddress Address) const;

private:
	// Filter object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> FilterObject;

	// Sort object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> SortObject;

	Faerie::FItemStorageQueryEvent OnQueryChanged;

	Faerie::Container::FVariantPredicate FilterFunction;
	Faerie::Container::FVariantComparator SortFunction;

	bool InvertFilter = false;
	bool InvertSort = false;
};