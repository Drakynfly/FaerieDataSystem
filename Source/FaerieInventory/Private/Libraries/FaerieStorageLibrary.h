// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieFunctionTemplates.h"
#include "FaerieItemContainerStructs.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieStorageLibrary.generated.h"

struct FFaerieItemDataViewWrapper;
class IFaerieItemOwnerInterface;
class UFaerieItem;
class UFaerieItemStackProxy;
class UFaerieItemStorage;

// We need to expose this delegate to the global namespace or UHT will cry.
using FFaerieViewPredicate = UFaerieFunctionTemplates::FFaerieViewPredicate;

/**
 * 
 */
UCLASS()
class UFaerieStorageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Storage Library")
	static const UFaerieItem* GetViewItem(const FFaerieItemDataViewWrapper& View);

	UFUNCTION(BlueprintPure, Category = "Faerie|Storage Library")
	static int32 GetViewCopies(const FFaerieItemDataViewWrapper& View);

	UFUNCTION(BlueprintPure, Category = "Faerie|Storage Library")
	static TScriptInterface<IFaerieItemOwnerInterface> GetViewOwner(const FFaerieItemDataViewWrapper& View);

	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Library")
	static TArray<UFaerieItemStackProxy*> GetAllStackProxies(UFaerieItemStorage* Storage);

	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Faerie|Storage Library")
	static FFaerieAddress QueryFirst(UFaerieItemStorage* Storage, const UFaerieFunctionTemplates::FFaerieViewPredicate& Filter);
};
