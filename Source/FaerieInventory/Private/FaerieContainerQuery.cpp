// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerQuery.h"
#include "DelegateCommon.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemDataComparator.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieContainerQuery)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorageQuery, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Query (First)"), STAT_Storage_QueryFirst, STATGROUP_FaerieItemStorageQuery);
DECLARE_CYCLE_STAT(TEXT("Query (All)"), STAT_Storage_QueryAll, STATGROUP_FaerieItemStorageQuery);

using namespace Faerie;

bool UFaerieContainerQuery::IsSortBound() const
{
	return SortFunction.IsBound();
}

bool UFaerieContainerQuery::IsFilterBound() const
{
	return FilterFunction.IsBound();
}

void UFaerieContainerQuery::SetFilter(ItemData::FViewPredicate&& Predicate, const UObject* AssociatedUObject)
{
	if (Predicate.IsBound())
	{
		FilterFunction = MoveTemp(Predicate);
		FilterObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetFilter();
	}
}

void UFaerieContainerQuery::SetFilterByDelegate(const FFaerieViewPredicate& Delegate)
{
	if (Delegate.IsBound())
	{
		// This works by assuming that the 'Faerie::ItemData::FValidatedDataView' parameter of the filter function is
		// invisible to the Delegate's parameter type of 'const FFaerieItemDataView&'.
		FilterFunction = DYNAMIC_TO_NATIVE(ItemData::FViewPredicate, Delegate);
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetFilter();
	}
}

void UFaerieContainerQuery::SetFilterByObject(const UFaerieItemDataFilter* Object)
{
	if (Object != FilterObject)
	{
		FilterFunction = ItemData::FViewPredicate::CreateUObject(Object, &UFaerieItemDataFilter::Exec);
		FilterObject = Object;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetFilter();
	}
}

void UFaerieContainerQuery::SetSort(ItemData::FViewComparator&& Comparator, const UObject* AssociatedUObject)
{
	if (Comparator.IsBound())
	{
		SortFunction = MoveTemp(Comparator);
		SortObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetSort();
	}
}

void UFaerieContainerQuery::SetSortByDelegate(const UFaerieFunctionTemplates::FFaerieViewComparator& Delegate)
{
	if (Delegate.IsBound())
	{
		SortFunction = DYNAMIC_TO_NATIVE(ItemData::FViewComparator, Delegate);
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetSort();
	}
}

void UFaerieContainerQuery::SetSortByObject(const UFaerieItemDataComparator* Comparator)
{
	if (Comparator != SortObject)
	{
		SortObject = Comparator;
		SortFunction = ItemData::FViewComparator::CreateUObject(Comparator, &UFaerieItemDataComparator::Exec);
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetSort();
	}
}

void UFaerieContainerQuery::SetInvertFilter(const bool Invert)
{
	if (Invert != InvertFilter)
	{
		InvertFilter = Invert;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieContainerQuery::SetInvertSort(const bool Invert)
{
	if (Invert != InvertSort)
	{
		InvertSort = Invert;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieContainerQuery::ResetFilter()
{
	if (IsFilterBound())
	{
		FilterFunction.Unbind();
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieContainerQuery::ResetSort()
{
	if (IsSortBound())
	{
		SortFunction.Unbind();
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

FFaerieAddress UFaerieContainerQuery::QueryFirstAddress(const UFaerieItemContainerBase* Container) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryFirst);

	if (!IsValid(Container))
	{
		return {};
	}

	if (!IsFilterBound()) return {};

	Container::FCallbackFilter IteratorPredicate{
		ItemData::FViewPredicate::CreateUObject(this, &ThisClass::IsIteratorFiltered)};

	if (InvertFilter)
	{
		return Container::FAddressFilter()
			   .Invert()
			   .By(MoveTemp(IteratorPredicate))
			   .First(Container);
	}

	return Container::FAddressFilter()
		   .By(MoveTemp(IteratorPredicate))
		   .First(Container);
}

void UFaerieContainerQuery::QueryAllAddresses(const UFaerieItemContainerBase* Container, TArray<FFaerieAddress>& OutAddresses) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryAll);

	if (!IsValid(Container))
	{
		return;
	}

	// Ensure we are starting with a blank slate.
	OutAddresses.Empty();

	if (IsFilterBound())
	{
		Container::FCallbackFilter IteratorPredicate{
			ItemData::FViewPredicate::CreateUObject(this, &ThisClass::IsIteratorFiltered)};

		if (InvertFilter)
		{
			OutAddresses = Container::FAddressFilter()
						   .Invert()
						   .By(MoveTemp(IteratorPredicate))
						   .Emit(Container);
		}
		else
		{
			OutAddresses = Container::FAddressFilter()
						   .By(MoveTemp(IteratorPredicate))
						   .Emit(Container);
		}
	}
	else
	{
		// If we have no filter, dump all addresses into the output.
		Container->GetAllAddresses(OutAddresses);
	}

	if (IsSortBound())
	{
		Algo::Sort(OutAddresses,
			[this, Container](const FFaerieAddress A, const FFaerieAddress B)
			{
				return CompareAddresses_Impl(Container, A, B);
			});
	}
}

bool UFaerieContainerQuery::CompareAddresses(const UFaerieItemContainerBase* Container, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	if (!IsValid(Container)) return false;

	if (SortFunction.IsBound())
	{
		return CompareAddresses_Impl(Container, AddressA, AddressB);
	}

	return false;
}

bool UFaerieContainerQuery::IsAddressFiltered(const UFaerieItemContainerBase* Container, const FFaerieAddress Address) const
{
	if (!IsValid(Container)) return false;

	if (FilterFunction.IsBound())
	{
		if (const FFaerieItemDataView StackView = Container->ViewAddress(Address))
		{
			if (InvertFilter)
			{
				return !FilterFunction.Execute(Container, StackView);
			}
			return FilterFunction.Execute(Container, StackView);
		}
	}

	return false;
}

bool UFaerieContainerQuery::CompareAddresses_Impl(const TNotNull<const UFaerieItemContainerBase*> Container,
												  const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	const FFaerieItemDataView StackViewA = Container->ViewAddress(AddressA);
	const FFaerieItemDataView StackViewB = Container->ViewAddress(AddressB);

	if (StackViewA && StackViewB)
	{
		if (InvertSort)
		{
			return !SortFunction.Execute(Container, StackViewA, StackViewB);
		}

		return SortFunction.Execute(Container, StackViewA, StackViewB);
	}
	return false;
}

bool UFaerieContainerQuery::IsIteratorFiltered(const TNotNull<const UObject*> WorldContextObj, const ItemData::FValidatedDataView& Iterator) const
{
	return FilterFunction.Execute(WorldContextObj, Iterator);
}
