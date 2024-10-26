﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Algo/BinarySearch.h"
#include "Algo/Sort.h"
#include "Containers/Array.h"

/**
 * This is a template base for types that wrap a struct array where the struct contains a Key and Value pair.
 * The goal of this template is to accelerate lookups/addition/removal with binary search.
 * This is to somewhat alleviate performance concerns when using TArrays for things that should be TMaps in networked
 * situations that forbid the latter.
 * See FInventoryContent for an example of this implemented.
 * TArrayType must implement a function with the signature `TArray<TElementType>& GetArray()`, and TElementType must have two
 * members Key, and Value. The Key type must have operator< implemented.
 */
template <typename TArrayType, typename TElementType>
struct FAERIEINVENTORY_API TBinarySearchOptimizedArray
{
	using BSOA = TBinarySearchOptimizedArray;
	using KeyType = decltype(TElementType::Key);
	using ValueType = decltype(TElementType::Value);

private:
	FORCEINLINE const TArray<TElementType>& GetArray_Internal() const { return const_cast<TArrayType*>(static_cast<const TArrayType*>(this))->GetArray(); }
	FORCEINLINE TArray<TElementType>& GetArray_Internal() { return static_cast<TArrayType*>(this)->GetArray(); }

public:
	int32 IndexOf(const KeyType Key) const
	{
		// Search for Key in the Items. Since those do not share Type, a projection is provided, in the form of the address
		// of the appropriate member to compare against. Finally the custom Predicate is provided.
		return Algo::BinarySearchBy(GetArray_Internal(), Key, &TElementType::Key);
	}

	bool Contains(KeyType Key) const
	{
		return IndexOf(Key) != INDEX_NONE;
	}

	const ValueType* Find(KeyType Key) const
	{
		const int32 Index = IndexOf(Key);

		if (Index != INDEX_NONE)
		{
			return &GetArray_Internal()[Index].Value;
		}
		return nullptr;
	}

	const TElementType& GetElement(const KeyType Key) const
	{
		return GetArray_Internal()[IndexOf(Key)];
	}

	ValueType& operator[](const KeyType Key)
	{
		return GetArray_Internal()[IndexOf(Key)].Value;
	}

	const ValueType& operator[](const KeyType Key) const
	{
		return GetArray_Internal()[IndexOf(Key)].Value;
	}

	KeyType GetKeyAt(int32 Index) const
	{
		if (!GetArray_Internal().IsValidIndex(Index))
		{
			return KeyType();
		}

		return GetArray_Internal()[Index].Key;
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
	 * be to simple Add/Emplace, and call Sort once aafterwards.
	 */
	TElementType& Insert(const TElementType& Element)
	{
		// Find the index of either ahead of where Key currently is, or where it should be inserted if it isn't present.
		const int32 NextIndex = Algo::UpperBoundBy(GetArray_Internal(), Element.Key, &TElementType::Key);

		if (NextIndex < GetArray_Internal().Num())
		{
			// Check if the index-1 is our key, and overwrite the data there if so.
			if (TElementType& CurrentEntry = GetArray_Internal()[NextIndex-1];
				CurrentEntry.Key == Element.Key)
			{
				CurrentEntry.Value = Element.Value;
				return CurrentEntry;
			}
		}

		// Otherwise, we were given a key not present and we should insert the Entry at the Index.
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
	bool Remove(KeyType Key, Predicate Pred)
	{
		if (const int32 Index = IndexOf(Key);
			Index != INDEX_NONE)
		{
			Pred(GetArray_Internal()[Index]);

			// RemoveAtSwap would be faster but would break Entry key order.
			GetArray_Internal().RemoveAt(Index);
			return true;
		}
		return false;
	}
};