// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

struct FInventoryEntry;
struct FInventoryKey;
struct FKeyedInventoryEntry;

// Note: This namespace is private, as FInventoryEntry and FKeyedInventoryEntry are not meant for external usage.
namespace Faerie::Inventory
{
	/* Sorts an array of inventory entries by their modified data. */
	FAERIEINVENTORY_API void SortEntriesLastModified(TArray<FInventoryEntry>& Entries);

	/* Break an array of inventory entries into inventory keys. (Entry & array{Stacks} -> array{Entry, Stack}) */
	FAERIEINVENTORY_API void BreakKeyedEntriesIntoInventoryKeys(const TArray<FKeyedInventoryEntry>& Entries, TArray<FInventoryKey>& OutKeys);
}