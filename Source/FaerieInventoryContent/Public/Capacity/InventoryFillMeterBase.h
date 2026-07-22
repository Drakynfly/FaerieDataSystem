// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "InventoryFillMeterBase.generated.h"

class UInventoryCapacityExtension;

/**
 * A widget responsible for displaying capacity information from a UInventoryCapacityExtension module.
 */
UCLASS(Abstract)
class FAERIEINVENTORYCONTENT_API UInventoryFillMeterBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void InitWithExtension(UInventoryCapacityExtension* Extension);

	UFUNCTION(BlueprintCallable)
	virtual void InitWithoutExtension();

	UFUNCTION(BlueprintCallable)
	virtual void Reset();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Fill Meter")
	void OnInit();

	UFUNCTION(BlueprintImplementableEvent, Category = "Fill Meter")
	void OnInitEmpty();

	UFUNCTION(BlueprintImplementableEvent, Category = "Fill Meter")
	void OnReset();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Fill Meter")
	TWeakObjectPtr<UInventoryCapacityExtension> CapacityModule;
};