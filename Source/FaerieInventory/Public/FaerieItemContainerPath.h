// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerPath.generated.h"

class UFaerieItemContainerBase;

/**
 * A Storage Path is a set of ItemContainers that form a path through nested sub-storage.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieItemContainerPath
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FaerieStoragePath")
	TArray<TObjectPtr<UFaerieItemContainerBase>> Containers;

	// Build all paths recursively from a parent (head) container.
	static void BuildChildrenPaths(UFaerieItemContainerBase* Head, TArray<FFaerieItemContainerPath>& OutPaths);

	// Build the path from a child to its parent-most container.
	static FFaerieItemContainerPath BuildParentPath(UFaerieItemContainerBase* Tail);

	UFaerieItemContainerBase* GetHead() const { return Containers.IsEmpty() ? nullptr : Containers[0]; }
	UFaerieItemContainerBase* GetTail() const { return Containers.IsEmpty() ? nullptr : Containers[Containers.Num()-1]; }
};
