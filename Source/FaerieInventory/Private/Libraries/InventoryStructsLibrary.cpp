// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "InventoryStructsLibrary.h"
#include "FaerieInventoryStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryStructsLibrary)

bool UInventoryStructsLibrary::Equal_EntryEntry(const FInventoryEntry& A, const FInventoryEntry& B, const int32 Checks)
{
	return FInventoryEntry::IsEqualTo(A, B, static_cast<EEntryEquivalencyFlags>(Checks));
}

void UInventoryStructsLibrary::SortEntriesLastModified(TArray<FInventoryEntry>& Entries)
{
	Faerie::Inventory::SortEntriesLastModified(Entries);
}

int32 UInventoryStructsLibrary::GetStackSum(const FInventoryEntry& Entry)
{
	return Entry.StackSum();
}

FFaerieItemStackView UInventoryStructsLibrary::EntryToStackView(const FInventoryEntry& Entry)
{
	return Entry.ToItemStackView();
}

bool UInventoryStructsLibrary::EqualEqual_InventoryKey(const FInventoryKey A, const FInventoryKey B)
{
	return A == B;
}

bool UInventoryStructsLibrary::EqualEqual_StackKey(const FStackKey A, const FStackKey B)
{
	return A == B;
}