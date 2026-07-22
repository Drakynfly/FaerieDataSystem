// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"
#include "ItemContainerEvent.h"
#include "FaerieStorageEnums.h"
#include "FaerieStorageStructs.h"

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
	friend FFaerieStorageContent;

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
	virtual void InitializeNetObject(TNotNull<AActor*> Actor) override;
	virtual void DeinitializeNetObject(TNotNull<AActor*> Actor) override;
	//~ UNetSupportedObject

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData) override;

	virtual bool Contains(FFaerieAddress Address) const override;
	virtual FFaerieItemInstance ViewInstance(FFaerieEntryKey Key) const override;
	virtual FFaerieItemInstance ViewInstance(FFaerieAddress Address) const override;
	virtual FFaerieItemDataView ViewEntry(FFaerieEntryKey Key) const override;
	virtual FFaerieItemDataView ViewAddress(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const override;
	virtual void DestroyStack(FFaerieEntryKey Key, int32 Copies) override;
	virtual void DestroyStack(FFaerieAddress Address, int32 Copies) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieEntryKey Key, int32 Copies) override;
	virtual TOptional<FFaerieUnownedItemStack> Release(FFaerieAddress Address, int32 Copies) override;
	virtual bool CanPossess(const FFaerieItemDataView& View) const override;
	virtual void GetAllAddresses(TArray<FFaerieAddress>& Addresses) const override;

protected:
	virtual TUniquePtr<Faerie::Container::IEntryIterator> CreateEntryIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateAddressIterator() const override;
	virtual TUniquePtr<Faerie::Container::IAddressIterator> CreateSingleEntryIterator(FFaerieEntryKey Key) const override;
	//~ UFaerieItemContainerBase

public:
	//~ IFaerieItemOwnerInterface
	virtual void DestroyStack(const FFaerieItemProxy& Proxy, int32 Copies = Faerie::ItemData::EntireStack) override;
	virtual bool Possess(const FFaerieUnownedItemStack& Stack) override;
	virtual void OnItemDataChanged(const Faerie::ItemData::FMutableReference& Instance, TNotNull<const UScriptStruct*> Struct, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/
private:
	[[nodiscard]] TArray<FFaerieEntryKey> CopyEntryKeys() const;

	// Collect all addresses for an entry into an array.
	[[nodiscard]] static TArray<FFaerieAddress> CollateAddresses(const FFaerieStorageEntry& Entry);

	[[nodiscard]] const FFaerieStorageEntry* GetEntrySafe(FFaerieEntryKey Key) const;

	[[nodiscard]] const FFaerieStorageEntry* FindEntry(const Faerie::ItemData::FReference& Item) const;

	[[nodiscard]] UFaerieItemStackProxy* GetStackProxyImpl(FFaerieAddress Address) const;

	// Internal implementation for adding items.
	[[nodiscard]] Faerie::Inventory::FEventData AddStackImplNoBroadcast(const Faerie::ItemData::FValidatedDataView& View, bool ForceNewStack);
	[[nodiscard]] Faerie::Inventory::FEventData AddStackImpl(const Faerie::ItemData::FValidatedDataView& View, bool ForceNewStack);

	// Internal implementations for removing items, specifying an amount.
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromEntryImplNoBroadcast(const FFaerieStorageEntry& Entry, int32 Amount);
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromStackImplNoBroadcast(const FFaerieStorageEntry& Entry, FFaerieStackKey Stack, int32 Amount);
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromEntryImpl(const FFaerieStorageEntry& Entry, int32 Amount, FFaerieInventoryTag Reason);
	[[nodiscard]] Faerie::Inventory::FEventData RemoveFromStackImpl(const FFaerieStorageEntry& Entry, FFaerieStackKey Stack, int32 Amount, FFaerieInventoryTag Reason);

	bool CanEditStackImpl(const FFaerieStorageEntry& Entry, FFaerieStackKey Stack, FFaerieInventoryTag EditTag) const;
	bool CanRemoveEntryImpl(const FFaerieStorageEntry& Entry, FFaerieInventoryTag Reason) const;
	bool CanRemoveStackImpl(const FFaerieStorageEntry& Entry, FFaerieStackKey Stack, FFaerieInventoryTag Reason) const;

	void Client_PostContentAdded(const FFaerieStorageEntry& Entry);
	void Client_PreContentRemoved(const FFaerieStorageEntry& Entry);
	void Client_PostContentChanged(const FFaerieStorageEntry& Entry);


	/**------------------------------*/
	/*	  STORAGE API - ALL USERS    */
	/**------------------------------*/
public:
	static FFaerieAddress MakeAddress(FFaerieEntryKey Entry, FFaerieStackKey Stack);
	static FFaerieEntryKey GetAddressEntry(FFaerieAddress Address);
	static FFaerieStackKey GetAddressStack(FFaerieAddress Address);
	static TTuple<FFaerieEntryKey, FFaerieStackKey> BreakAddress(FFaerieAddress Address);

	// Gets the number of items stacked at the address.
	int32 GetStack(FFaerieAddress Address) const;

	// Get the stack proxy object for an address.
	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	const UFaerieItemStackProxy* GetProxy(FFaerieAddress Address) const;

	// Breaks an address into an entry and stack key, verifying that they are valid.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Access")
	bool BreakAddressIntoKeys(FFaerieAddress Address, FFaerieEntryKey& Entry, FFaerieStackKey& Stack) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Access")
	TArray<FFaerieStackKey> BreakEntryIntoKeys(FFaerieEntryKey Key) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Access")
	TArray<int32> GetStacksInEntry(FFaerieEntryKey Key) const;

	// Gets all the addresses stored to an Entry Key.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Access")
	TArray<FFaerieAddress> GetAddressesForEntry(FFaerieEntryKey Key) const;

	// Gets all entry keys contained in this storage. They are in sorted order.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Access")
	void GetAllKeys(TArray<FFaerieEntryKey>& Keys) const;

	// Retrieve the number of entries in storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	int32 GetEntryCount() const;

	// Retrieve the number of stacks in storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
    int32 GetStackCount() const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	bool ContainsKey(FFaerieEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	bool ContainsAddress(FFaerieAddress Address) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	bool ContainsItem(const FFaerieItemInstance& Item) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	FFaerieEntryKey FindItem(const FFaerieItemInstance& Item) const;

	// Utility function mainly used with inventories that are expected to only contain a single entry, e.g., pickups.
	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	FFaerieAddress GetFirstAddress() const;

	// Gets the item stored at an entry.
	UFUNCTION(BlueprintCallable, Category = "Storage|Access")
	FFaerieItemInstance GetEntryItem(FFaerieEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanAddStack(const FFaerieItemDataView& View, EFaerieStorageAddStackBehavior AddStackBehavior) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanAddStacks(const TArray<FFaerieItemDataView>& Views, FFaerieExtensionAllowsAdditionArgs Args) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanEditStack(FFaerieAddress Address, FFaerieInventoryTag EditTag) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanRemoveEntry(FFaerieEntryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Permissions")
	bool CanRemoveStack(FFaerieAddress Address,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

	// Add a single item instance into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|")
	bool AddEntryFromInstance(const FFaerieItemInstance& Instance, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add an item stack into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool AddItemStack(const FFaerieUnownedItemStack& Stack, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add an item stack into storage, and return the full data about the change.
	void AddItemStack(const FFaerieUnownedItemStack& Stack, EFaerieStorageAddStackBehavior AddStackBehavior, TValueOrError<Faerie::Inventory::FEventData, FText>& OutResult);

	bool AddItemStacks(TConstArrayView<FFaerieUnownedItemStack> Stacks, EFaerieStorageAddStackBehavior AddStackBehavior, bool StopAfterFailure);

protected:
	// Add an item stack into storage.
	UFUNCTION(BlueprintCallable, Category = "Storage")
	void AddItemStackBulk(const TArray<FFaerieUnownedItemStack>& Stacks, EFaerieStorageAddStackBehavior AddStackBehavior, bool StopAfterFailure);

	// Add an item stack into storage, and return the full data about the change. Blueprint callable version that returns a wrapped event log.
	UFUNCTION(BlueprintCallable, Category = "Storage", DisplayName = "Add Item Stack (with Log)")
	bool AddItemStackWithLog(const FFaerieUnownedItemStack& Stack, EFaerieStorageAddStackBehavior AddStackBehavior, FFaerieBlueprintInventoryEvent& Event);

public:
	/**
	 * Removes the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool RemoveEntry(FFaerieEntryKey Key,
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
	bool TakeEntry(FFaerieEntryKey Key, FFaerieUnownedItemStack& OutStack,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes and returns the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool TakeStack(FFaerieAddress Address, FFaerieUnownedItemStack& OutStack,
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
	FFaerieEntryKey MoveStack(UFaerieItemStorage* ToStorage, FFaerieAddress Address, int32 Amount = -1,
		EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack);

	UFUNCTION(BlueprintCallable, Category = "Storage")
	FFaerieEntryKey MoveEntry(UFaerieItemStorage* ToStorage, FFaerieEntryKey Key, EFaerieStorageAddStackBehavior AddStackBehavior);

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool MergeStacks(FFaerieEntryKey Entry, FFaerieStackKey FromStack, FFaerieStackKey ToStack, int32 Amount = -1);

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
	FFaerieStorageContent EntryMap;

	// Locally stored proxies per entry stack.
	// These properties are transient, mainly so that editor code that accesses them doesn't need to worry about Caches
	// being left around. Using weak pointers here is intentional. We don't want this storage to keep these alive. They
	// should be stored in a strong pointer by whatever requested them, and once nothing needs the proxies, they will die.
	TMap<FFaerieAddress, TWeakObjectPtr<UFaerieItemStackProxy>> LocalStackProxies;
};