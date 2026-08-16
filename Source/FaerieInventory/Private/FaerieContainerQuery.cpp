// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerQuery.h"
#include "DelegateCommon.h"
#include "EntityManagerHelpers.h"
#include "FaerieContainerFilter.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemDataComparator.h"
#include "FaerieItemStorage.h"
#include "FaerieItemTemplate.h"

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

void UFaerieContainerQuery::SetFilterByDelegate(const FFaerieProxyPredicate& Delegate)
{
	if (Delegate.IsBound())
	{
		// This works by assuming that the 'Faerie::TValid<const FFaerieItemProxy&>' parameter of the filter function is
		// invisible to the Delegate's parameter type of 'const FFaerieItemProxy&'.
		FilterFunction = DYNAMIC_TO_NATIVE(ItemData::FViewPredicate, Delegate);
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else
	{
		ResetFilter();
	}
}

void UFaerieContainerQuery::SetFilterByObject(const UFaerieItemTemplate* Object)
{
	if (Object != FilterObject)
	{
		FilterFunction = ItemData::FViewPredicate::CreateUObject(Object, &UFaerieItemTemplate::TryMatch);
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

void UFaerieContainerQuery::SetSortByDelegate(const UFaerieFunctionTemplates::FFaerieProxyComparator& Delegate)
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

	auto* EntityManager = ItemData::GetFaerieEntityManager();

	if (InvertFilter)
	{
		return Container::FAddressFilter()
			   .Invert()
			   .By(MoveTemp(IteratorPredicate))
			   .First(EntityManager, Container);
	}

	return Container::FAddressFilter()
		   .By(MoveTemp(IteratorPredicate))
		   .First(EntityManager, Container);
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
		auto* EntityManager = ItemData::GetFaerieEntityManager();

		Container::FCallbackFilter IteratorPredicate{
			ItemData::FViewPredicate::CreateUObject(this, &ThisClass::IsIteratorFiltered)};

		if (InvertFilter)
		{
			OutAddresses = Container::FAddressFilter()
						   .Invert()
						   .By(MoveTemp(IteratorPredicate))
						   .Emit(EntityManager, Container);
		}
		else
		{
			OutAddresses = Container::FAddressFilter()
						   .By(MoveTemp(IteratorPredicate))
						   .Emit(EntityManager, Container);
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
		const ItemData::FScopeProxy StackView = Container->ViewAddress(Address);
		const FFaerieItemProxy Proxy(FFaerieItemProxy::ESingleFrame, &StackView);

		if (Proxy.IsValid())
		{
			auto* EntityManager = ItemData::GetFaerieEntityManager();

			if (InvertFilter)
			{
				return !FilterFunction.Execute(EntityManager, Proxy);
			}
			return FilterFunction.Execute(EntityManager, Proxy);
		}
	}

	return false;
}

bool UFaerieContainerQuery::CompareAddresses_Impl(const TNotNull<const UFaerieItemContainerBase*> Container,
												  const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	const ItemData::FScopeProxy StackViewA = Container->ViewAddress(AddressA);
	const ItemData::FScopeProxy StackViewB = Container->ViewAddress(AddressB);
	const FFaerieItemProxy ProxyA(FFaerieItemProxy::ESingleFrame, &StackViewA);
	const FFaerieItemProxy ProxyB(FFaerieItemProxy::ESingleFrame, &StackViewB);

	if (ProxyA.IsValid() && ProxyB.IsValid())
	{
		auto* EntityManager = ItemData::GetFaerieEntityManager();

		if (InvertSort)
		{
			return !SortFunction.Execute(EntityManager, ProxyA, ProxyB);
		}

		return SortFunction.Execute(EntityManager, ProxyA, ProxyB);
	}
	return false;
}

bool UFaerieContainerQuery::IsIteratorFiltered(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Iterator) const
{
	return FilterFunction.Execute(EntityManager, Iterator);
}
