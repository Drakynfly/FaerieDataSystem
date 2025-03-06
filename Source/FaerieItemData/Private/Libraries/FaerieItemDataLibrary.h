// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieItemDataLibrary.generated.h"

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
	// If the item needs to allow changes, enable MutableInstance.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemAsset")
	static UFaerieItem* GetItemInstance(const UFaerieItemAsset* Asset, bool MutableInstance);

	// Spits out an item in Json form for debugging.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemAsset", meta = (DevelopmentOnly))
	static FString DebugEmitItemJson(const UFaerieItem* Item, bool Pretty);
};