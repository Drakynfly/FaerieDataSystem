// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "InventoryUIActionContainer.generated.h"

class UInventoryUIAction2;

/**
 * Contains an Array of UI actions.
 */
UCLASS(BlueprintType)
class FAERIEINVENTORYCONTENT_API UInventoryUIActionContainer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionOfClass(TSubclassOf<UInventoryUIAction2> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionInstance(UInventoryUIAction2* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionOfClass(TSubclassOf<UInventoryUIAction2> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionInstance(UInventoryUIAction2* Action);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "ActionContainer")
	TArray<TObjectPtr<UInventoryUIAction2>> Actions;
};
