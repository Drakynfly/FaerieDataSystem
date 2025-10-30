// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "FaerieItemStorage.h"
#include "FaerieStorageWidgetBase.generated.h"

class UFaerieItemStorageQuery;
class UInventoryUIActionContainer;

/**
 *
 */
UCLASS(Abstract)
class FAERIEINVENTORYCONTENT_API UFaerieStorageWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFaerieStorageWidgetBase(const FObjectInitializer& ObjectInitializer);

	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void Reset();
	virtual void HandleAddressEvent(UFaerieItemStorage* Storage, const EFaerieAddressEventType Type, FFaerieAddress Address);

public:
	// Set the inventory that will be used when this widget is constructed.
	UFUNCTION(BlueprintCallable, Category = "Inventory Contents|Config")
	void SetLinkedStorage(UFaerieItemStorage* Storage);

	/**
	 * This must be called *after* this widget is added to viewport. Initialization of child widgets cannot be performed
	 * if called while not on-screen.
	 * Calling this multiple times with the same inventory component is intended, and has no performance hit.
	 * It is not necessary to call this at all, unless switching inventories. Calling SetLinkedInventory when created is
	 * sufficient.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory Contents|Config")
	void InitWithInventory(UFaerieItemStorage* Storage);

	UFUNCTION(BlueprintCallable, Category = "Inventory Contents|Display")
	void AddToSortOrder(FFaerieAddress Address, bool WarnIfAlreadyExists);

	UFUNCTION(BlueprintCallable, Category = "Inventory Contents|Display")
	void RequestResort();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void OnInitWithInventory();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void DisplaySortedEntries();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void OnReset();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void OnKeyAdded(FFaerieAddress Address);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void OnKeyUpdated(FFaerieAddress Address);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Contents|Display")
	void OnKeyRemoved(FFaerieAddress Address);


	/// ***		SETUP		*** ///
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Config")
	TObjectPtr<UInventoryUIActionContainer> ActionContainer;

	// By default, all newly added items are sorted into the display order. Disable this when customizing order or filter
	// with OnEntryAdded.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool bAlwaysAddNewToSortOrder = true;


	/// ***		RUNTIME		*** ///

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TArray<FFaerieAddress> SortedAndFilteredAddresses;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TObjectPtr<UFaerieItemStorageQuery> StorageQuery;

	/** The storage this widget is representing.  */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

private:
	bool NeedsResort = false;
	bool NeedsReconstructEntries = false;
};