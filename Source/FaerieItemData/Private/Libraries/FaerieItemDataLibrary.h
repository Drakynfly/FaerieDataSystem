// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataEnums.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieItemDataLibrary.generated.h"

class FBlueprintTokenEdit;
struct FFaerieItemEditHandle;
class UFaerieItem;
class UFaerieItemAsset;
class UFaerieItemToken;

/**
 *
 */
UCLASS()
class UFaerieItemDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemDataLibrary")
	static bool Equal_ItemData(const UFaerieItem* A, const UFaerieItem* B);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemDataLibrary")
	static bool Equal_ItemToken(const UFaerieItemToken* A, const UFaerieItemToken* B);

	// Get the item instance this asset represents. By default, this will return the immutable asset if possible.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemAsset")
	static UFaerieItem* GetItemInstance(const UFaerieItemAsset* Asset, const EFaerieItemInstancingMutability Mutability);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemInstance")
	static UFaerieItem* NewItemInstance(const TArray<UFaerieItemToken*>& Tokens, EFaerieItemInstancingMutability Mutability);

	// Try to get access to the editing API of a faerie item. This will only succeed if the item is Mutable, meaning
	// that the item instance is not an asset-reference, and that token mutability was enabled during instancing.
	// If neither is true, and an item still needs to be modifiable by runtime code, a duplicate of the item must be made
	// and swapped with the non-mutable instance.
	// If trying to edit an item instanced at runtime, ensure that Mutability is enabled during creation.
	UFUNCTION(BlueprintCallable, Category = "Faerie|EditHandle", meta = (ExpandBoolAsExecs = "ReturnValue"))
	static bool TryGetEditHandle(const UFaerieItem* Item, FFaerieItemEditHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Faerie|EditHandle")
	static bool IsValidHandle(const FFaerieItemEditHandle& Handle);

	UFUNCTION(BlueprintPure, Category = "Faerie|EditHandle")
	static UFaerieItem* GetItem(const FFaerieItemEditHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EditHandle")
	static bool AddToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EditHandle")
	static bool RemoveToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token);

	UFUNCTION(BlueprintCallable, Category = "Faerie|EditHandle")
	static int32 RemoveTokensByClass(const FFaerieItemEditHandle& Handle, TSubclassOf<UFaerieItemToken> Class);

	/** Attempt to modify this token. Pass in a predicate that performs the edit. */
	// @todo remove this as well, this is to hard to enforce safe usage. Just add UFUNC setters/getters on tokens that can mutate
	UFUNCTION(BlueprintCallable, Category = "Faerie|EditHandle")
	static bool EditToken(const FFaerieItemEditHandle& Handle, UFaerieItemToken* Token, const FBlueprintTokenEdit& Edit);

	// Spits out an item in Json form for debugging.
	UFUNCTION(BlueprintPure, Category = "Faerie|Debug", meta = (DevelopmentOnly))
	static FString DebugEmitItemJson(const UFaerieItem* Item, bool Pretty);

	// Compare two items by their Json form.
	UFUNCTION(BlueprintPure, Category = "Faerie|Debug", meta = (DevelopmentOnly))
	static bool DebugCompareItemsByJson(const UFaerieItem* ItemA, const UFaerieItem* ItemB);
};