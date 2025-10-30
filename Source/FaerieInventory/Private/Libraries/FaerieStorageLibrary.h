// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieFunctionTemplates.h"
#include "FaerieItemContainerStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieStorageLibrary.generated.h"

class UFaerieItemStorage;

// We need to expose this delegate to the global namespace or UHT will cry.
using FFaerieSnapshotPredicate = UFaerieFunctionTemplates::FFaerieSnapshotPredicate;

/**
 * 
 */
UCLASS()
class FAERIEINVENTORY_API UFaerieStorageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Library")
	static FFaerieAddress QueryFirst(UFaerieItemStorage* Storage, const UFaerieFunctionTemplates::FFaerieSnapshotPredicate& Filter);
};
