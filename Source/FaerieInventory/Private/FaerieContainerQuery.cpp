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

using namespace Faerie::ItemData;
using namespace Faerie::Container;

bool UFaerieContainerQuery::IsSortBound() const
{
	return SortFunction.IsBound();
}

bool UFaerieContainerQuery::IsFilterBound() const
{
	return FilterFunction.IsBound();
}

void UFaerieContainerQuery::SetFilter(FViewPredicate&& Predicate, const UObject* AssociatedUObject)
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
		// This works by assuming that the 'const Faerie::ItemData::FViewPtr&' parameter of the filter function is
		// invisible to the Delegate's parameter type of 'const FFaerieItemDataViewWrapper&'.
		FilterFunction = DYNAMIC_TO_NATIVE(FViewPredicate, Delegate);
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
		FilterFunction = FViewPredicate::CreateUObject(Object, &UFaerieItemDataFilter::ExecView);
		FilterObject = Object;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetFilter();
	}
}

void UFaerieContainerQuery::SetSort(FViewComparator&& Comparator, const UObject* AssociatedUObject)
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
		SortFunction = DYNAMIC_TO_NATIVE(FViewComparator, Delegate);
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
		SortFunction = FViewComparator::CreateUObject(Comparator, &UFaerieItemDataComparator::Exec);
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

	FCallbackFilter IteratorPredicate{
		FIteratorPredicate::CreateUObject(this, &ThisClass::IsIteratorFiltered)};

	if (InvertFilter)
	{
		return FAddressFilter()
			.Invert()
			.By(MoveTemp(IteratorPredicate))
			.First(Container);
	}

	return FAddressFilter()
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
		FCallbackFilter IteratorPredicate{
			FIteratorPredicate::CreateUObject(this, &ThisClass::IsIteratorFiltered)};

		if (InvertFilter)
		{
			OutAddresses = FAddressFilter()
				.Invert()
				.By(MoveTemp(IteratorPredicate))
				.Emit(Container);
		}
		else
		{
			OutAddresses = FAddressFilter()
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

namespace
{
	struct FImmediateView final : public IViewBase
	{
		FImmediateView(const TNotNull<const IFaerieItemOwnerInterface*> Owner, const FFaerieItemStackView Stack)
			: Owner(Owner), Stack(Stack) {}

		virtual bool IsValid() const override { return Stack.IsValid(); }
		virtual const UFaerieItem* ResolveItem() const override { return Stack.Item.Get(); }
		virtual FFaerieItemStackView ResolveView() const override { return Stack; }
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const override { return Owner; }

		const TNotNull<const IFaerieItemOwnerInterface*> Owner;
		const FFaerieItemStackView Stack;
	};
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
		FImmediateView View(Container, Container->ViewStack(Address));

		if (InvertFilter)
		{
			return !FilterFunction.Execute(&View);
		}
		return FilterFunction.Execute(&View);
	}

	return false;
}

bool UFaerieContainerQuery::CompareAddresses_Impl(const TNotNull<const UFaerieItemContainerBase*> Container,
													const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	const FImmediateView ViewA(Container, Container->ViewStack(AddressA));
	const FImmediateView ViewB(Container, Container->ViewStack(AddressB));

	if (InvertSort)
	{
		return !SortFunction.Execute(&ViewA, &ViewB);
	}

	return SortFunction.Execute(&ViewA, &ViewB);
}

bool UFaerieContainerQuery::IsIteratorFiltered(FIteratorPtr Iterator) const
{
	return FilterFunction.Execute(Iterator);
}
