// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieUIActionBase.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "InventoryUIActionContainer.generated.h"

/**
 * Contains an Array of UI actions.
 */
UCLASS(BlueprintType, EditInlineNew)
class FAERIEINVENTORYCONTENT_API UInventoryUIActionContainer : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddAction(TSubclassOf<UFaerieUIActionBase> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActions(TSet<TSubclassOf<UFaerieUIActionBase>> Classes);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddActionInstance(UFaerieUIActionBase* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool AddSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveAction(TSubclassOf<UFaerieUIActionBase> Class);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActions(TSet<TSubclassOf<UFaerieUIActionBase>> Classes);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveActionInstance(UFaerieUIActionBase* Action);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	bool RemoveSubContainer(UInventoryUIActionContainer* Container);

	UFUNCTION(BlueprintCallable, Category = "UIActionContainer")
	TArray<UFaerieUIActionBase*> GetAllActions() const;

protected:
	// Actions that run on the CDO
	UPROPERTY(EditAnywhere, Category = "UIActionContainer")
	TSet<TSubclassOf<UFaerieUIActionBase>> ActionClasses;

	// Actions that run on an instance
	UPROPERTY(EditAnywhere, Instanced, Category = "UIActionContainer")
	TArray<TObjectPtr<UFaerieUIActionBase>> ActionInstances;

	UPROPERTY()
	TArray<TObjectPtr<UInventoryUIActionContainer>> SubContainers;
};
