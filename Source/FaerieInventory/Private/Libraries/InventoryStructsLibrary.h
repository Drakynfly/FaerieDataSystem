// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "InventoryDataStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryStructsLibrary.generated.h"

/**
 * Library for exposing struct and inventory functions to blueprint.
 */
UCLASS()
class UInventoryStructsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
    static bool IsValid(const FEntryKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static FString ToString(const FEntryKey Key) { return Key.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static bool IsValid_InventoryKey(const FInventoryKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static FString ToString_InventoryKey(const FInventoryKey Key) { return Key.ToString(); }

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Inventory Key)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_InventoryKey(FInventoryKey A, FInventoryKey B);

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Stack Key)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_StackKey(FStackKey A, FStackKey B);

	// DEPRECATED

	// Selectively equivalate two entries.
	UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "=="), meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out"))
	static bool Equal_EntryEntry(const FInventoryEntry& A, const FInventoryEntry& B,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/FaerieInventory.EEntryEquivalencyFlags")) int32 Checks);

	// Sort an array of inventory entries by date modified.
	UFUNCTION(BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out"))
	static void SortEntriesLastModified(UPARAM(ref)TArray<FInventoryEntry>& Entries);

	UFUNCTION(BlueprintPure, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out"))
	static int32 GetStackSum(const FInventoryEntry& Entry);

	UFUNCTION(BlueprintPure, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out"))
	static FFaerieItemStackView EntryToStackView(const FInventoryEntry& Entry);
};