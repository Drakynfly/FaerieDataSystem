// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "FaerieItemStorage.h"
#include "FaerieStorageWidgetBase.generated.h"

class UFaerieContainerQuery;
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

	void OnPostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events);

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

	// Adds an item to the SortedAndFilteredAddresses list. Returns the index it was added at, or INDEX_NONE if it wasn't added.
	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	int32 AddToSortOrder(FFaerieAddress Address, bool WarnIfAlreadyExists);

	// Flag this widget to re-query its content next frame.
	UFUNCTION(BlueprintCallable, Category = "Faerie|StorageWidget")
	void RequestQuery();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnInitWithInventory();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void DisplayAddresses();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnReset();

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressAdded(FFaerieAddress Address, int32 IndexInDisplay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressUpdated(FFaerieAddress Address, int32 IndexInDisplay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Faerie|StorageWidget")
	void OnAddressRemoved(FFaerieAddress Address, int32 IndexInDisplay);


	/// ***		SETUP		*** ///
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category = "Config")
	TObjectPtr<UInventoryUIActionContainer> ActionContainer;

	// When enabled, this widget will attempt to bind to an events extension to update live with changes to the storage.
	// Disable this if this widget binds to a storage object without dynamic updates.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
	bool EnableUpdateEvents = true;


	/// ***		RUNTIME		*** ///

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TArray<FFaerieAddress> SortedAndFilteredAddresses;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TObjectPtr<UFaerieContainerQuery> StorageQuery;

	/** The storage this widget is representing.  */
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

private:
	bool NeedsNewQuery = false;
	bool NeedsReDisplay = false;
};