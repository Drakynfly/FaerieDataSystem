// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Algo/BinarySearch.h"
#include "Algo/IsSorted.h"
#include "Algo/Sort.h"
#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("FaerieDataUtils"), STATGROUP_FaerieDataUtils, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BSOA Index Of"), STAT_BSOA_IndexOf, STATGROUP_FaerieDataUtils);

/**
 * This is a template base for types that wrap a struct array where the struct contains a Key.
 * The goal of this template is to accelerate lookups/addition/removal with binary search.
 * This is to somewhat alleviate performance concerns when using TArrays for things that should be TMaps in networked
 * situations that forbid the latter.
 * See FInventoryContent for an example of this implemented.
 * TArrayType must implement a function with the signature `TArray<TElementType>& GetArray()`, and TElementType must have a
 * member named Key. The Key type must have operator< implemented.
 */
template <typename TArrayType, typename TElementType>
struct TBinarySearchOptimizedArray
{
	using BSOA = TBinarySearchOptimizedArray;
	using KeyType = decltype(TElementType::Key);

private:
	FORCEINLINE const TArray<TElementType>& GetArray_Internal() const { return const_cast<TArrayType*>(static_cast<const TArrayType*>(this))->GetArray(); }
	FORCEINLINE TArray<TElementType>& GetArray_Internal() { return static_cast<TArrayType*>(this)->GetArray(); }

public:
	int32 IndexOf(const KeyType Key) const
	{
		checkf(IsSorted(), TEXT("Array got out of order. BinarySearch will not function. Determine why Array is not sorted!"));
		SCOPE_CYCLE_COUNTER(STAT_BSOA_IndexOf);
		// Search for Key in the Items. Since those do not share Type, we project by the element key.
		return Algo::BinarySearchBy(GetArray_Internal(), Key, &TElementType::Key);
	}

	FORCEINLINE bool Contains(KeyType Key) const
	{
		return IndexOf(Key) != INDEX_NONE;
	}

	const TElementType* Find(KeyType Key) const
	{
		const int32 Index = IndexOf(Key);

		if (Index != INDEX_NONE)
		{
			return &GetArray_Internal()[Index];
		}
		return nullptr;
	}

	FORCEINLINE const TElementType& GetElement(const KeyType Key) const
	{
		return GetArray_Internal()[IndexOf(Key)];
	}

	FORCEINLINE TElementType& operator[](const KeyType Key)
	{
		return GetArray_Internal()[IndexOf(Key)];
	}

	FORCEINLINE const TElementType& operator[](const KeyType Key) const
	{
		return GetArray_Internal()[IndexOf(Key)];
	}

	FORCEINLINE KeyType GetKeyAt(int32 Index) const
	{
		return GetArray_Internal()[Index].Key;
	}

	FORCEINLINE const TElementType& GetElementAt(int32 Index) const
	{
		return GetArray_Internal()[Index];
	}

	/**
	 * Force a full resort of the array. Must be called whenever there are changes made to the array without
	 * verifying sort order. Example use case is mass addition of elements of unknown order. Instead of sorting each one
	 * as they are added, it's more efficient to add them all in whatever order they came in, or perform one sort after.
	 */
	void Sort()
	{
		Algo::SortBy(GetArray_Internal(), &TElementType::Key);
	}

	/**
	 * Performs a binary search to find where to insert this new key. Needed when Key is not guaranteed to be sequential,
	 * otherwise, a simple Add would suffice.
	 * In performance-critical code it would be slower to use Insert when adding multiple items in one scope, better would
	 * be to simply Add/Emplace, and call Sort once afterwards.
	 */
	TElementType& Insert(const TElementType& Element)
	{
		// Find the index of either ahead of where Key currently is, or where it should be inserted if it isn't present.
		const int32 NextIndex = Algo::UpperBoundBy(GetArray_Internal(), Element.Key, &TElementType::Key);

		if (GetArray_Internal().IsValidIndex(NextIndex-1))
		{
			// Check if the index-1 is our key, and overwrite the data there if so.
			if (TElementType& CurrentEntry = GetArray_Internal()[NextIndex-1];
				CurrentEntry.Key == Element.Key)
			{
				CurrentEntry = Element;
				return CurrentEntry;
			}
		}

		// Otherwise, we were given a key not present, and we should insert the Entry at the Index.
		return GetArray_Internal().Insert_GetRef(Element, NextIndex);
	}

	bool Remove(KeyType Key)
	{
		if (const int32 Index = IndexOf(Key);
			Index != INDEX_NONE)
		{
			// Notify owning server of this removal.
			GetArray_Internal().RemoveAt(Index);
			return true;
		}
		return false;
	}

	template <typename Predicate>
	bool Remove(KeyType Key, Predicate PreRemovalPredicate)
	{
		if (const int32 Index = IndexOf(Key);
			Index != INDEX_NONE)
		{
			PreRemovalPredicate(GetArray_Internal()[Index]);

			// RemoveAtSwap would be faster but would break Entry key order.
			GetArray_Internal().RemoveAt(Index);
			return true;
		}
		return false;
	}

	// Debug function for checking if we are out of order
	bool IsSorted() const
	{
		return Algo::IsSortedBy(GetArray_Internal(), &TElementType::Key);
	}
};