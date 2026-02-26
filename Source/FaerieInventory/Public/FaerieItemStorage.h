// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"
#include "ItemContainerEvent.h"
#include "FaerieItemStack.h"
#include "InventoryDataEnums.h"
#include "InventoryDataStructs.h"

#include "FaerieItemStorage.generated.h"

struct FFaerieExtensionAllowsAdditionArgs;
class UFaerieItemStackProxy;

namespace Faerie::Storage
{
	class FStorageDataAccess;
}

/**
 *
 */
UCLASS(BlueprintType)
class FAERIEINVENTORY_API UFaerieItemStorage : public UFaerieItemContainerBase
{
	GENERATED_BODY()

	// Allow the struct that contains our item data to call our content change notification functions.
	friend FInventoryContent;

	// Allow iterators and filters to read our data.
	friend Faerie::Storage::FStorageDataAccess;

public:
	//~ UObject
	virtual void PostInitProperties() override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoad() override;
	//~ UObject

	//~ UNetSupportedObject
	virtual void InitializeNetObject(AActor* Actor) override;
	virtual void DeinitializeNetObject(AActor* Actor) override;
	//~ UNetSupportedObject

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData) override;
	virtual bool Contains(FEntryKey Key) const override;
	virtual FFaerieItemStackView View(FEntryKey Key) const override;
	virtual FFaerieItemStack Release(FEntryKey Key, int32 Copies) override;
	virtual int32 GetStack(FEntryKey Key) const override;

	virtual void GetAllAddresses(TArray<FFaerieAddress>& Addresses) const override;
	virtual bool Contains(FFaerieAddress Address) const override;
	virtual int32 GetStack(FFaerieAddress Address) const override;
	virtual const UFaerieItem* ViewItem(FEntryKey Key) const override;
	virtual const UFaerieItem* ViewItem(FFaerieAddress Address) const override;
	virtual FFaerieItemStackView ViewStack(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const override;
	virtual FFaerieItemStack Release(FFaerieAddress Address, int32 Copies) override;

private:
	virtual TUniquePtr<Faerie::Container::IIterator> CreateEntryIterator() const override;
	virtual TUniquePtr<Faerie::Container::IIterator> CreateAddressIterator() const override;
	virtual TUniquePtr<Faerie::Container::IIterator> CreateSingleEntryIterator(FEntryKey Key) const override;
	//~ UFaerieItemContainerBase

public:
	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;

protected:
	virtual void OnItemMutated(TNotNull<const UFaerieItem*> Item, TNotNull<const UFaerieItemToken*> Token, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/
private:
	[[nodiscard]] TArray<FEntryKey> CopyEntryKeys() const;

	[[nodiscard]] const FInventoryEntry* GetEntrySafe(FEntryKey Key) const;

	[[nodiscard]] const FInventoryEntry* FindEntry(const TNotNull<const UFaerieItem*> Item, EFaerieItemEqualsCheck Method) const;

	[[nodiscard]] UFaerieItemStackProxy* GetStackProxyImpl(FFaerieAddress Address) const;

	// Internal implementation for adding items.
	[[nodiscard]] Faerie::Inventory::FEventData AddStackImplNoBroadcast(const FFaerieItemStack& InStack, bool ForceNewStack);
	[[nodiscard]] Faerie::Inventory::FEventData AddStackImpl(const FFaerieItemStack& InStack, bool ForceNewStack);

	// Internal implementations for removing items, specifying an amount.
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromEntryImplNoBroadcast(const FInventoryEntry& Entry, int32 Amount);
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromEntryImpl(const FInventoryEntry& Entry, int32 Amount, FFaerieInventoryTag Reason);
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromStackImpl(FFaerieAddress Address, int32 Amount, FFaerieInventoryTag Reason);

	bool CanRemoveEntryImpl(const FInventoryEntry& Entry, FFaerieInventoryTag Reason) const;

	void PostContentAdded(const FInventoryEntry& Entry);
	void PreContentRemoved(const FInventoryEntry& Entry);
	void PostContentChanged(const FInventoryEntry& Entry, FInventoryContent::EChangeType ChangeType, const TBitArray<>* EntryChangeMask);


	/**------------------------------*/
	/*	  STORAGE API - ALL USERS    */
	/**------------------------------*/
public:
	static FFaerieAddress MakeAddress(FEntryKey Entry, FStackKey Stack);
	static FEntryKey GetAddressEntry(FFaerieAddress Address);
	static FStackKey GetAddressStack(FFaerieAddress Address);
	static TTuple<FEntryKey, FStackKey> BreakAddress(FFaerieAddress Address);

	// Breaks an address into a entry and stack key, verifying that they are valid.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	bool BreakAddressIntoKeys(FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	TArray<FStackKey> BreakEntryIntoKeys(FEntryKey Key) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	TArray<int32> GetStacksInEntry(FEntryKey Key) const;

	// Gets all the addresses stored to an Entry Key.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	TArray<FFaerieAddress> GetAddressesForEntry(FEntryKey Key) const;

	// Gets all entry keys contained in this storage. They are in sorted order.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	void GetAllKeys(TArray<FEntryKey>& Keys) const;

	// Retrieve the number of entries in storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	int32 GetEntryCount() const;

	// Retrieve the number of stacks in storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
    int32 GetStackCount() const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	bool ContainsKey(FEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool ContainsItem(const UFaerieItem* Item, EFaerieItemEqualsCheck Method) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	FEntryKey FindItem(const UFaerieItem* Item, EFaerieItemEqualsCheck Method) const;

	// Utility function mainly used with inventories that are expected to only contain a single entry, e.g., pickups.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	FFaerieAddress GetFirstAddress() const;

	// Gets the item stored at an entry.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	const UFaerieItem* GetEntryItem(FEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanAddStack(FFaerieItemStackView Stack, EFaerieStorageAddStackBehavior AddStackBehavior) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanAddStacks(const TArray<FFaerieItemStackView>& Stacks, FFaerieExtensionAllowsAdditionArgs Args) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanEditEntry(FEntryKey Key, FFaerieInventoryTag EditTag) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanEditStack(FFaerieAddress Address, FFaerieInventoryTag EditTag) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanRemoveEntry(FEntryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanRemoveStack(FFaerieAddress Address,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

	// Add a single raw item into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool AddEntryFromItemObject(const UFaerieItem* ItemObject, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add an item stack into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool AddItemStack(const FFaerieItemStack& ItemStack, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add an item stack into storage, and return the full data about the change.
	void AddItemStack(const FFaerieItemStack& ItemStack, EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Faerie::Inventory::FEventData, FText>& OutResult);

	void AddItemStacks(TConstArrayView<FFaerieItemStack> ItemStacks, EFaerieStorageAddStackBehavior AddStackBehavior);

protected:
	// Add an item stack into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage")
	void AddItemStackBulk(const TArray<FFaerieItemStack>& ItemStacks, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add an item stack into storage, and return the full data about the change. Blueprint callable version that returns a wrapped event log.
	UFUNCTION(BlueprintCallable, Category = "Storage", DisplayName = "Add Item Stack (with Log)")
	bool AddItemStackWithLog(const FFaerieItemStack& ItemStack, EFaerieStorageAddStackBehavior AddStackBehavior, FLoggedInventoryEvent& Event);

public:
	/**
	 * Removes the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool RemoveEntry(FEntryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool RemoveStack(FFaerieAddress Address,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes and returns the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool TakeEntry(FEntryKey Key, FFaerieItemStack& OutStack,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes and returns the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool TakeStack(FFaerieAddress Address, FFaerieItemStack& OutStack,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Clear out the entire contents of the storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
    void Clear(UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag);

	/**
	 * Add an entry from this storage to another, then remove it from this one, optionally move only part of a stack.
	 * @return The key used by the ToStorage to store the entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	FEntryKey MoveStack(UFaerieItemStorage* ToStorage, FFaerieAddress Address, int32 Amount = -1,
		EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack);

	UFUNCTION(BlueprintCallable, Category = "Storage")
	FEntryKey MoveEntry(UFaerieItemStorage* ToStorage, FEntryKey Key, EFaerieStorageAddStackBehavior AddStackBehavior);

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool MergeStacks(FEntryKey Entry, FStackKey FromStack, FStackKey ToStack, int32 Amount = -1);

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool SplitStack(FFaerieAddress Address, int32 Amount);

	/** Call MoveEntry on all entries in this storage. */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	void Dump(UFaerieItemStorage* ToStorage);


	/**-------------*/
	/*	 VARIABLES	*/
	/**-------------*/
private:
	// Our internal data containing the contents of the storage.
	UPROPERTY(Replicated)
	FInventoryContent EntryMap;

	// Locally stored proxies per entry stack.
	// These properties are transient, mainly so that editor code that accesses them doesn't need to worry about Caches
	// being left around. Using weak pointers here is intentional. We don't want this storage to keep these alive. They
	// should be stored in a strong pointer by whatever requested them, and once nothing needs the proxies, they will die.
	UPROPERTY(Transient)
	TMap<FFaerieAddress, TWeakObjectPtr<UFaerieItemStackProxy>> LocalStackProxies;
};