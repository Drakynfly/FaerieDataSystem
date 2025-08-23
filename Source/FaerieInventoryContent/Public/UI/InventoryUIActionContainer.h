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
	bool AddAction(TSubclassOf<UInventoryUIAction> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActions(TSet<TSubclassOf<UInventoryUIAction>> Classes);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionInstance(UInventoryUIAction* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveAction(TSubclassOf<UInventoryUIAction> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActions(TSet<TSubclassOf<UInventoryUIAction>> Classes);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionInstance(UInventoryUIAction* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	TArray<UInventoryUIAction*> GetAllActions() const;

protected:
	// Actions that run on the CDO
	UPROPERTY()
	TSet<TSubclassOf<UInventoryUIAction>> ActionClasses;

	// Actions that run on an instance
	UPROPERTY()
	TArray<TObjectPtr<UInventoryUIAction>> ActionInstances;

	UPROPERTY()
	TArray<TObjectPtr<UInventoryUIActionContainer>> SubContainers;
};
