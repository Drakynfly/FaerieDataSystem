// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageQuery.h"
#include "DelegateCommon.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemDataComparator.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemStorage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorageQuery)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorageQuery, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Query (First)"), STAT_Storage_QueryFirst, STATGROUP_FaerieItemStorageQuery);
DECLARE_CYCLE_STAT(TEXT("Query (All)"), STAT_Storage_QueryAll, STATGROUP_FaerieItemStorageQuery);

using namespace Faerie::ItemData;
using namespace Faerie::Container;

bool UFaerieItemStorageQuery::IsSortBound() const
{
	return SortFunction.IsBound();
}

bool UFaerieItemStorageQuery::IsFilterBound() const
{
	return FilterFunction.IsBound();
}

void UFaerieItemStorageQuery::SetFilter(FViewPredicate&& Predicate, const UObject* AssociatedUObject)
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

void UFaerieItemStorageQuery::SetFilterByDelegate(const FFaerieViewPredicate& Delegate)
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

void UFaerieItemStorageQuery::SetFilterByObject(const UFaerieItemDataFilter* Object)
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

void UFaerieItemStorageQuery::SetSort(FViewComparator&& Comparator, const UObject* AssociatedUObject)
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

void UFaerieItemStorageQuery::SetSortByDelegate(const UFaerieFunctionTemplates::FFaerieViewComparator& Delegate)
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

void UFaerieItemStorageQuery::SetSortByObject(const UFaerieItemDataComparator* Comparator)
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

void UFaerieItemStorageQuery::SetInvertFilter(const bool Invert)
{
	if (Invert != InvertFilter)
	{
		InvertFilter = Invert;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetInvertSort(const bool Invert)
{
	if (Invert != InvertSort)
	{
		InvertSort = Invert;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::ResetFilter()
{
	if (IsFilterBound())
	{
		FilterFunction.Unbind();
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::ResetSort()
{
	if (IsSortBound())
	{
		SortFunction.Unbind();
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

FFaerieAddress UFaerieItemStorageQuery::QueryFirstAddress(const UFaerieItemStorage* Storage) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryFirst);

	if (!IsValid(Storage))
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
			.First(Storage);
	}

	return FAddressFilter()
		   .By(MoveTemp(IteratorPredicate))
		   .First(Storage);
}

void UFaerieItemStorageQuery::QueryAllAddresses(const UFaerieItemStorage* Storage, TArray<FFaerieAddress>& OutAddresses) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryAll);

	if (!IsValid(Storage))
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
				.Emit(Storage);
		}
		else
		{
			OutAddresses = FAddressFilter()
				.By(MoveTemp(IteratorPredicate))
				.Emit(Storage);
		}
	}
	else
	{
		// If we have no filter, dump all addresses into the output.
		Storage->GetAllAddresses(OutAddresses);
	}

	if (IsSortBound())
	{
		Algo::Sort(OutAddresses,
			[this, Storage](const FFaerieAddress A, const FFaerieAddress B)
			{
				return CompareAddresses(Storage, A, B);
			});
	}
}

namespace
{
	struct FImmediateView final : public Faerie::ItemData::IViewBase
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

bool UFaerieItemStorageQuery::CompareAddresses(const UFaerieItemStorage* Storage, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	if (!IsValid(Storage)) return false;

	if (SortFunction.IsBound())
	{
		const FImmediateView ViewA(Storage, Storage->ViewStack(AddressA));
		const FImmediateView ViewB(Storage, Storage->ViewStack(AddressB));
		return SortFunction.Execute(&ViewA, &ViewB);
	}

	return false;
}

bool UFaerieItemStorageQuery::IsAddressFiltered(const UFaerieItemStorage* Storage, const FFaerieAddress Address) const
{
	if (!IsValid(Storage)) return false;

	if (FilterFunction.IsBound())
	{
		FImmediateView View(Storage, Storage->ViewStack(Address));

		if (InvertFilter)
		{
			return !FilterFunction.Execute(&View);
		}
		return FilterFunction.Execute(&View);
	}

	return false;
}

bool UFaerieItemStorageQuery::IsIteratorFiltered(FIteratorPtr Iterator) const
{
	if (FilterFunction.IsBound())
	{
		return FilterFunction.Execute(Iterator);
	}
	return false;
}
