// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieSubObjectLibrary.generated.h"

class UFaerieItem;
class UFaerieItemContainerBase;

/**
 * 
 */
UCLASS()
class UFaerieSubObjectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Get all container objects from ContainerTokens.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects")
	static TArray<UFaerieItemContainerBase*> GetAllContainersInItem(UFaerieItem* Item, bool Recursive);

	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects", meta = (DeterminesOutputType = Class))
	static TArray<UFaerieItemContainerBase*> GetContainersInItemOfClass(UFaerieItem* Item, TSubclassOf<UFaerieItemContainerBase> Class, bool Recursive);

	// Get all child items from an item.
	UFUNCTION(BlueprintCallable, Category = "Faerie|SubObjects")
	static TArray<UFaerieItem*> GetItemChildren(UFaerieItem* Item, bool Recursive);
};
