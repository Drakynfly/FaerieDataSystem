// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemStorageQuery.h"
#include "FaerieContainerFilterTypes.h"
#include "FaerieFunctionTemplates.h"
#include "FaerieItemDataComparator.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemStorage.h"
#include "FaerieItemStorageIterators.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemStorageQuery)

DECLARE_STATS_GROUP(TEXT("FaerieItemStorage"), STATGROUP_FaerieItemStorageQuery, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Query (First)"), STAT_Storage_QueryFirst, STATGROUP_FaerieItemStorageQuery);
DECLARE_CYCLE_STAT(TEXT("Query (All)"), STAT_Storage_QueryAll, STATGROUP_FaerieItemStorageQuery);

bool UFaerieItemStorageQuery::IsSortBound() const
{
	return SortFunction.GetIndex() != 0;
}

bool UFaerieItemStorageQuery::IsFilterBound() const
{
	return FilterFunction.GetIndex() != 0;
}

void UFaerieItemStorageQuery::SetFilter(Faerie::Container::FItemPredicate&& Predicate, UObject* AssociatedUObject)
{
	if (Predicate.IsSet())
	{
		FilterFunction.Emplace<Faerie::Container::FItemPredicate>(MoveTemp(Predicate));
		FilterObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilter(Faerie::Container::FStackPredicate&& Predicate, UObject* AssociatedUObject)
{
	if (Predicate.IsSet())
	{
		FilterFunction.Emplace<Faerie::Container::FStackPredicate>(MoveTemp(Predicate));
		FilterObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilter(Faerie::Container::FSnapshotPredicate&& Predicate, UObject* AssociatedUObject)
{
	if (Predicate.IsSet())
	{
		FilterFunction.Emplace<Faerie::Container::FSnapshotPredicate>(MoveTemp(Predicate));
		FilterObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilterByDelegate_Item(const FFaerieItemPredicate& Delegate)
{
	if (Delegate.IsBound())
	{
		FilterFunction.Emplace<Faerie::Container::FItemPredicate>([Delegate](const UFaerieItem* Item)
			{
				return Delegate.Execute(Item);
			});
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsFilterBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilterByDelegate_Stack(const FFaerieStackPredicate& Delegate)
{
	if (Delegate.IsBound())
	{
		FilterFunction.Emplace<Faerie::Container::FStackPredicate>([Delegate](const FFaerieItemStackView& Stack)
			{
				return Delegate.Execute(Stack);
			});
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsFilterBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilterByDelegate_Snapshot(const FFaerieSnapshotPredicate& Delegate)
{
	if (Delegate.IsBound())
	{
		FilterFunction.Emplace<Faerie::Container::FSnapshotPredicate>([Delegate](const FFaerieItemSnapshot& Snapshot)
			{
				return Delegate.Execute(Snapshot);
			});
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsFilterBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetFilterByObject(const UFaerieItemDataFilter* Object)
{
	if (Object != FilterObject)
	{
		FilterFunction.Emplace<Faerie::Container::FSnapshotPredicate>([Object](const FFaerieItemSnapshot& Snapshot)
			{
				const FFaerieItemStackView View{Snapshot.ItemObject, Snapshot.Copies};
				return Object->Exec(View);
			});
		FilterObject = Object;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsFilterBound())
	{
		FilterFunction.Emplace<FEmptyVariantState>(); // Reset Filter
		FilterObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSort(Faerie::Container::FItemComparator&& Comparator, UObject* AssociatedUObject)
{
	if (Comparator.IsSet())
	{
		SortFunction.Emplace<Faerie::Container::FItemComparator>(MoveTemp(Comparator));
		SortObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSort(Faerie::Container::FStackComparator&& Comparator, UObject* AssociatedUObject)
{
	if (Comparator.IsSet())
	{
		SortFunction.Emplace<Faerie::Container::FStackComparator>(MoveTemp(Comparator));
		SortObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSort(Faerie::Container::FSnapshotComparator&& Comparator, UObject* AssociatedUObject)
{
	if (Comparator.IsSet())
	{
		SortFunction.Emplace<Faerie::Container::FSnapshotComparator>(MoveTemp(Comparator));
		SortObject = AssociatedUObject;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSortByDelegate_Item(const FFaerieItemComparator& Delegate)
{
	if (Delegate.IsBound())
	{
		SortFunction.Emplace<Faerie::Container::FItemComparator>(
			[Delegate](const UFaerieItem* ItemA, const UFaerieItem* ItemB)
			{
				return Delegate.Execute(ItemA, ItemB);
			});
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSortByDelegate_Stack(const FFaerieStackComparator& Delegate)
{
	if (Delegate.IsBound())
	{
		SortFunction.Emplace<Faerie::Container::FStackComparator>(
			[Delegate](const FFaerieItemStackView& StackA, const FFaerieItemStackView& StackB)
			{
				return Delegate.Execute(StackA, StackB);
			});
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSortByDelegate_Snapshot(const FFaerieSnapshotComparator& Delegate)
{
	if (Delegate.IsBound())
	{
		SortFunction.Emplace<Faerie::Container::FSnapshotComparator>(
			[Delegate](const FFaerieItemSnapshot& SnapshotA, const FFaerieItemSnapshot& SnapshotB)
			{
				return Delegate.Execute(SnapshotA, SnapshotB);
			});
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
	}
}

void UFaerieItemStorageQuery::SetSortByObject(const UFaerieItemDataComparator* Comparator)
{
	if (Comparator != SortObject)
	{
		SortObject = Comparator;
		SortFunction.Emplace<Faerie::Container::FSnapshotComparator>(
			[Comparator](const FFaerieItemSnapshot& SnapshotA, const FFaerieItemSnapshot& SnapshotB)
			{
				return Comparator->Exec(SnapshotA, SnapshotB);
			});
		OnQueryChanged.Broadcast(this);
	}
	else if (IsSortBound())
	{
		SortFunction.Emplace<FEmptyVariantState>(); // Reset Sort
		SortObject = nullptr;
		OnQueryChanged.Broadcast(this);
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

FFaerieAddress UFaerieItemStorageQuery::QueryFirstAddress(const UFaerieItemStorage* Storage) const
{
	SCOPE_CYCLE_COUNTER(STAT_Storage_QueryFirst);

	if (!IsFilterBound()) return {};

	if (InvertFilter)
	{
		return Storage->QueryFirst([this, Storage](const FFaerieAddress& Address)
			{
				return !IsAddressFiltered(Storage, Address);
			});
	}

	return Storage->QueryFirst([this, Storage](const FFaerieAddress& Address)
		{
			return IsAddressFiltered(Storage, Address);
		});
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

	const bool HasFilter = IsFilterBound();
	const bool HasSort = IsSortBound();

	if (!HasFilter && !HasSort)
	{
		if (!InvertFilter)
		{
			for (const FFaerieAddress Address : Faerie::Storage::FIterator_AllAddresses(*Storage))
			{
				OutAddresses.Add(Address);
			}

			if (InvertSort)
			{
				Algo::Reverse(OutAddresses);
			}
		}
		return;
	}

	//Faerie::Storage::FItemFilter_Address Res(Storage);
	Faerie::Storage::FItemFilter_Key Res(Storage);

	if (HasFilter)
	{
		// @todo temporarily disabled filters, while Address filters don't work.
		Faerie::Container::FAddressFilterCallback AddressFilter;
		AddressFilter.Callback = [this, Storage](const FFaerieAddress Address)
			{
				return IsAddressFiltered(Storage, Address);
			};
		//Res.Run(MoveTemp(AddressFilter));

		if (InvertFilter)
		{
			Res.Invert();
		}
	}

	if (HasSort)
	{
		if (InvertSort)
		{
			OutAddresses = Res.SortBy<Faerie::Container::ESortDirection::Backward>(SortFunction).EmitAddresses();
		}
		else
		{
			OutAddresses = Res.SortBy(SortFunction).EmitAddresses();
		}
	}
	else
	{
		OutAddresses = Res.EmitAddresses();
	}
}

FFaerieItemSnapshot MakeSnapshot(const UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
	const FFaerieItemStackView StorageA = Storage->ViewStack(Address);

	FFaerieItemSnapshot Snap;
	Snap.Owner = Storage;
	Snap.ItemObject = StorageA.Item.Get();
	Snap.Copies = StorageA.Copies;
	return Snap;
}

bool UFaerieItemStorageQuery::CompareAddresses(const UFaerieItemStorage* Storage, const FFaerieAddress AddressA, const FFaerieAddress AddressB) const
{
	switch (SortFunction.GetIndex())
	{
	case 1:
		// Compare Items
		{
			const UFaerieItem* ItemA = Storage->ViewItem(AddressA);
			const UFaerieItem* ItemB = Storage->ViewItem(AddressB);
			const bool Result = SortFunction.Get<Faerie::Container::FItemComparator>()(ItemA, ItemB);
			return InvertSort ? !Result : Result;
		}
	case 2:
		// Compare Stacks
		{
			const FFaerieItemStackView StackA = Storage->ViewStack(AddressA);
			const FFaerieItemStackView StackB = Storage->ViewStack(AddressB);
			const bool Result = SortFunction.Get<Faerie::Container::FStackComparator>()(StackA, StackB);
			return InvertSort ? !Result : Result;
		}
	case 3:
		// Compare Snapshots
		{
			const FFaerieItemSnapshot SnapA = MakeSnapshot(Storage, AddressA);
			const FFaerieItemSnapshot SnapB = MakeSnapshot(Storage, AddressB);
			const bool Result = SortFunction.Get<Faerie::Container::FSnapshotComparator>()(SnapA, SnapB);
			return InvertSort ? !Result : Result;
		}
	default:
		return false;
	}
}

bool UFaerieItemStorageQuery::IsAddressFiltered(const UFaerieItemStorage* Storage, const FFaerieAddress Address) const
{
	switch (FilterFunction.GetIndex())
	{
	case 1:
		// Filter Item
		{
			const UFaerieItem* Item = Storage->ViewItem(Address);
			const bool Result = FilterFunction.Get<Faerie::Container::FItemPredicate>()(Item);
			return InvertSort ? !Result : Result;
		}
	case 2:
		// Filter Stack
		{
			const FFaerieItemStackView Stack = Storage->ViewStack(Address);
			const bool Result = FilterFunction.Get<Faerie::Container::FStackPredicate>()(Stack);
			return InvertSort ? !Result : Result;
		}
	case 3:
		// Filter Snapshot
		{
			const FFaerieItemSnapshot Snapshot = MakeSnapshot(Storage, Address);
			const bool Result = FilterFunction.Get<Faerie::Container::FSnapshotPredicate>()(Snapshot);
			return InvertFilter ? Result : !Result;
		}
	default:
		return false;
	}
}
