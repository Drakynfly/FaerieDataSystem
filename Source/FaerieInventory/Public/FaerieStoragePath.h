// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieStoragePath.generated.h"

class UFaerieItemContainerBase;

/**
 * A Storage Path is a set of ItemContainers that form a path through nested sub-storage.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieStoragePath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FaerieStoragePath")
	TArray<TObjectPtr<UFaerieItemContainerBase>> Containers;

	UFaerieItemContainerBase* GetHead() const { return Containers.IsEmpty() ? nullptr : Containers[0]; }
	UFaerieItemContainerBase* GetTail() const { return Containers.IsEmpty() ? nullptr : Containers[Containers.Num()-1]; }
};
