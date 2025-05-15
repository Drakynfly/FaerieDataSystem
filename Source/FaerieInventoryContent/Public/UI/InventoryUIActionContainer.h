// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "InventoryUIActionContainer.generated.h"

class UInventoryUIAction;

/**
 * Contains an Array of UI actions.
 */
UCLASS(BlueprintType)
class FAERIEINVENTORYCONTENT_API UInventoryUIActionContainer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionOfClass(TSubclassOf<UInventoryUIAction> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionInstance(UInventoryUIAction* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionsOfClass(TSubclassOf<UInventoryUIAction> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionInstance(UInventoryUIAction* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	TArray<UInventoryUIAction*> GetAllActions() const;

protected:
	UPROPERTY()
	TArray<TObjectPtr<UInventoryUIAction>> Actions;

	UPROPERTY()
	TArray<TObjectPtr<UInventoryUIActionContainer>> SubContainers;
};
