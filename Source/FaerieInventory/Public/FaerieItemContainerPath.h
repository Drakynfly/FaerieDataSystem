// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemInstance.h"
#include "FaerieItemContainerPath.generated.h"

class UFaerieItemContainerBase;

USTRUCT(BlueprintType)
struct FFaerieItemContainerPathPair
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FaerieStoragePathPair")
	FFaerieItemInstance OwningInstance;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FaerieStoragePathPair")
	TObjectPtr<UFaerieItemContainerBase> Container;
};

/**
 * A Storage Path is a set of ItemContainers that form a path through nested sub-storage.
 */
USTRUCT(BlueprintType)
struct FAERIEINVENTORY_API FFaerieItemContainerPath
{
	GENERATED_BODY()

	// Contains UFaerieItemContainerBase or IFaerieItemDataProxy
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "FaerieStoragePath")
	TArray<FFaerieItemContainerPathPair> Containers;

	// Build all paths recursively from a parent (head) container.
	static void BuildChildrenPaths(Faerie::ItemData::FRequireEntityManager& EntityManager, const Faerie::ItemData::FMutableReference& Owner, TNotNull<UFaerieItemContainerBase*> Head, TArray<FFaerieItemContainerPath>& OutPaths);

	UFaerieItemContainerBase* GetHead() const { return Containers.IsEmpty() ? nullptr : Containers[0].Container; }
	UFaerieItemContainerBase* GetTail() const { return Containers.IsEmpty() ? nullptr : Containers[Containers.Num()-1].Container; }
};
