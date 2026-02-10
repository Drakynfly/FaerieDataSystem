// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataViewBase.h"
#include "UObject/Object.h"
#include "FaerieItemStorageQuery.generated.h"

class UFaerieItemDataComparator;
class UFaerieItemDataFilter;
class UFaerieItemStorage;
class UFaerieItemStorageQuery;

// We need to expose these delegates to the global namespace or UHT will cry.
using FFaerieViewPredicate = UFaerieFunctionTemplates::FFaerieViewPredicate;
using FFaerieViewComparator = UFaerieFunctionTemplates::FFaerieViewComparator;

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

	void SetFilter(Faerie::ItemData::FViewPredicate&& Predicate, const UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetFilterByDelegate(const UFaerieFunctionTemplates::FFaerieViewPredicate& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetFilterByObject(const UFaerieItemDataFilter* Object);

	void SetSort(Faerie::ItemData::FViewComparator&& Comparator, const UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetSortByDelegate(const UFaerieFunctionTemplates::FFaerieViewComparator& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetSortByObject(const UFaerieItemDataComparator* Comparator);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetInvertFilter(bool Invert);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void SetInvertSort(bool Invert);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void ResetFilter();

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void ResetSort();

	// Query function to filter and sort for a subsection of contained entries.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	FFaerieAddress QueryFirstAddress(const UFaerieItemStorage* Storage) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	void QueryAllAddresses(const UFaerieItemStorage* Storage, TArray<FFaerieAddress>& OutAddresses) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool CompareAddresses(const UFaerieItemStorage* Storage, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Query")
	bool IsAddressFiltered(const UFaerieItemStorage* Storage, const FFaerieAddress Address) const;

	bool IsIteratorFiltered(Faerie::Container::FIteratorPtr Iterator) const;

private:
	// Filter object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> FilterObject;

	// Sort object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> SortObject;

	Faerie::FItemStorageQueryEvent OnQueryChanged;

	Faerie::ItemData::FViewPredicate FilterFunction;
	Faerie::ItemData::FViewComparator SortFunction;

	bool InvertFilter = false;
	bool InvertSort = false;
};