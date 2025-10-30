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
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void Reset();
	virtual void HandleAddressEvent(UFaerieItemStorage* Storage, const EFaerieAddressEventType Type, TConstArrayView<FFaerieAddress> Addresses);

public:
	// Set the inventory that will be used when this widget is constructed.
	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	void SetLinkedStorage(UFaerieItemStorage* Storage);

	/**
	 * This must be called *after* this widget is added to viewport. Initialization of child widgets cannot be performed
	 * if called while not on-screen.
	 * Calling this multiple times with the same inventory component is intended, and has no performance hit.
	 * It is not necessary to call this at all, unless switching inventories. Calling SetLinkedInventory when created is
	 * sufficient.
	 */
	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	void InitWithInventory(UFaerieItemStorage* Storage);

	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	void AddToSortOrder(FFaerieAddress Address, bool WarnIfAlreadyExists);

	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	void RequestResort();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnInitWithInventory();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void DisplayAddresses();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnReset();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressAdded(FFaerieAddress Address);

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressUpdated(FFaerieAddress Address);

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressRemoved(FFaerieAddress Address);


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