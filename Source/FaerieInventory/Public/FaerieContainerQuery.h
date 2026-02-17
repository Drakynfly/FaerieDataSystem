// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataViewBase.h"
#include "UObject/Object.h"
#include "FaerieContainerQuery.generated.h"

class UFaerieItemDataComparator;
class UFaerieItemDataFilter;
class UFaerieContainerQuery;

// We need to expose these delegates to the global namespace or UHT will cry.
using FFaerieViewPredicate = UFaerieFunctionTemplates::FFaerieViewPredicate;
using FFaerieViewComparator = UFaerieFunctionTemplates::FFaerieViewComparator;

namespace Faerie
{
	using FContainerQueryEvent = TMulticastDelegate<void(const UFaerieContainerQuery*)>;
}

/**
 * A UObject wrapper around the Faerie::Container API.
 */
UCLASS(BlueprintType)
class FAERIEINVENTORY_API UFaerieContainerQuery : public UObject
{
	GENERATED_BODY()

public:
	Faerie::FContainerQueryEvent::RegistrationType& GetQueryChangedEvent() { return OnQueryChanged; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool IsSortBound() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool IsFilterBound() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool GetInvertFilter() const { return InvertFilter; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool GetInvertSort() const { return InvertSort; }

	void SetFilter(Faerie::ItemData::FViewPredicate&& Predicate, const UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetFilterByDelegate(const UFaerieFunctionTemplates::FFaerieViewPredicate& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetFilterByObject(const UFaerieItemDataFilter* Object);

	void SetSort(Faerie::ItemData::FViewComparator&& Comparator, const UObject* AssociatedUObject);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetSortByDelegate(const UFaerieFunctionTemplates::FFaerieViewComparator& Delegate);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetSortByObject(const UFaerieItemDataComparator* Comparator);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetInvertFilter(bool Invert);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void SetInvertSort(bool Invert);

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void ResetFilter();

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void ResetSort();

	// Query function to filter and sort for a subsection of contained entries.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	FFaerieAddress QueryFirstAddress(const UFaerieItemContainerBase* Container) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	void QueryAllAddresses(const UFaerieItemContainerBase* Container, TArray<FFaerieAddress>& OutAddresses) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool CompareAddresses(const UFaerieItemContainerBase* Container, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|Container Query")
	bool IsAddressFiltered(const UFaerieItemContainerBase* Container, const FFaerieAddress Address) const;

protected:
	bool CompareAddresses_Impl(TNotNull<const UFaerieItemContainerBase*> Container, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const;
	bool IsIteratorFiltered(Faerie::Container::FIteratorPtr Iterator) const;

private:
	// Filter object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> FilterObject;

	// Sort object to keep alive.
	UPROPERTY()
	TObjectPtr<const UObject> SortObject;

	Faerie::FContainerQueryEvent OnQueryChanged;

	Faerie::ItemData::FViewPredicate FilterFunction;
	Faerie::ItemData::FViewComparator SortFunction;

	bool InvertFilter = false;
	bool InvertSort = false;
};