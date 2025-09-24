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
    UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "Is Valid (Entry Key)")
    static bool IsValid_EntryKey(const FEntryKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "Is Valid (Stack Key)")
    static bool IsValid_StackKey(const FStackKey Key) { return Key.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Entry Key)")
	static FString ToString_EntryKey(const FEntryKey Key) { return Key.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Stack Key)")
	static FString ToString_StackKey(const FStackKey Key) { return Key.ToString(); }

	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils")
	static bool IsValid_Address(const FFaerieAddress Address) { return Address.IsValid(); }

	// @todo not safe to use. Only works for addresses from a UFaerieItemStorage.
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", DisplayName = "To String (Faerie Address)")
	static FString ToString_Address(FFaerieAddress Address);

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Faerie Address)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_Address(const FFaerieAddress A, const FFaerieAddress B) { return A == B; }

	/* Returns true if inventory keys are equal */
	UFUNCTION(BlueprintPure, Category = "Faerie|Inventory|Utils", meta = (DisplayName = "Equal (Stack Key)", CompactNodeTitle = "==", Keywords = "== equal compare", ScriptOperator = "=="))
	static bool EqualEqual_StackKey(FStackKey A, FStackKey B);
};