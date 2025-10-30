// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FaerieItemStackWidgetBase.generated.h"

class UFaerieStorageWidgetBase;
class UInventoryStackProxy;

/**
 * Responsible for displaying a single inventory entry in an entry list widget.
 */
UCLASS(Abstract)
class FAERIEINVENTORYCONTENT_API UFaerieItemStackWidgetBase : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemStackWidget")
	void SetInventoryWidget(UFaerieStorageWidgetBase* Widget);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "ItemStackWidget")
	TObjectPtr<UFaerieStorageWidgetBase> InventoryWidget;

	UPROPERTY(BlueprintReadWrite, Category = "ItemStackWidget")
	TObjectPtr<UInventoryStackProxy> LocalCache;
};