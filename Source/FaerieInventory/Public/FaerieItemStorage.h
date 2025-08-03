// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerBase.h"
#include "ItemContainerEvent.h"
#include "FaerieItemStack.h"
#include "InventoryDataEnums.h"
#include "InventoryDataStructs.h"

#include "FaerieItemStorage.generated.h"

struct FFaerieExtensionAllowsAdditionArgs;
DECLARE_LOG_CATEGORY_EXTERN(LogFaerieItemStorage, Log, All);

UENUM(BlueprintType)
enum class EFaerieAddressEventType : uint8
{
	// Broadcast whenever an address is added, or a stack amount is increased.
	PostAdd,

	// Broadcast whenever an address is removed entirely, or a stack amount is decreased.
	PreRemove,

	// Broadcast whenever data for an address is changed.
	Edit
};

namespace Faerie
{
	using FEntryKeyEvent = TMulticastDelegate<void(UFaerieItemStorage*, FEntryKey)>;

	using FAddressEvent = TMulticastDelegate<void(UFaerieItemStorage*, EFaerieAddressEventType, FFaerieAddress)>;
	using FStorageFilterFunc = TFunctionRef<bool(const FFaerieItemProxy&)>;
	using FStorageFilter = TDelegate<bool(const FFaerieItemProxy&)>;
	using FStorageComparator = TDelegate<bool(const FFaerieItemProxy&, const FFaerieItemProxy&)>;

	struct FStorageQuery
	{
		FStorageFilter Filter;
		bool InvertFilter = false;
		FStorageComparator Sort;
		bool InvertSort = false;
	};
}

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FBlueprintStorageFilter, const FFaerieItemProxy&, Proxy);
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FBlueprintStorageComparator, const FFaerieItemProxy&, A, const FFaerieItemProxy&, B);

USTRUCT(BlueprintType)
struct FFaerieItemStorageBlueprintQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "ItemStorageQuery")
	FBlueprintStorageFilter Filter;

	UPROPERTY(BlueprintReadWrite, Category = "ItemStorageQuery")
	bool InvertFilter = false;

	UPROPERTY(BlueprintReadWrite, Category = "ItemStorageQuery")
	FBlueprintStorageComparator Sort;

	UPROPERTY(BlueprintReadWrite, Category = "ItemStorageQuery")
	bool Reverse = false;
};

class UInventoryEntryProxy;
class UInventoryStackProxy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEntryKeyEvent, UFaerieItemStorage*, Storage, FEntryKey, Key);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFaerieAddressEvent, UFaerieItemStorage*, Storage, EFaerieAddressEventType, Type, FFaerieAddress, Key);

/**
 *
 */
UCLASS(BlueprintType)
class FAERIEINVENTORY_API UFaerieItemStorage : public UFaerieItemContainerBase
{
	GENERATED_BODY()

	// Allow the struct that contains our item data to call our content change notification functions.
	friend FInventoryContent;

public:
	//~ UObject
	virtual void PostInitProperties() override;
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
	virtual FFaerieItemProxy Proxy(FEntryKey Key) const override;
	virtual FFaerieItemStack Release(FEntryKey Key, int32 Copies) override;
	virtual void ForEachKey(Faerie::TLoop<FEntryKey> Func) const override;
	virtual int32 GetStack(FEntryKey Key) const override;
	virtual TArray<FFaerieAddress> Switchover_GetAddresses(FEntryKey Key) const override;

	virtual bool Contains(FFaerieAddress Address) const override;
	virtual int32 GetStack(FFaerieAddress Address) const override;
	virtual const UFaerieItem* ViewItem(FFaerieAddress Address) const override;
	virtual FFaerieItemStackView ViewStack(FFaerieAddress Address) const override;
	virtual FFaerieItemProxy Proxy(FFaerieAddress Address) const override;
	virtual FFaerieItemStack Release(FFaerieAddress Address, int32 Copies) override;
	virtual void ForEachAddress(Faerie::TLoop<FFaerieAddress> Func) const override;
	virtual void ForEachItem(Faerie::TLoop<const UFaerieItem*> Func) const override;
	//~ UFaerieItemContainerBase

	//~ IFaerieItemOwnerInterface
	virtual FFaerieItemStack Release(FFaerieItemStackView Stack) override;
	virtual bool Possess(FFaerieItemStack Stack) override;

protected:
	virtual void OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, FGameplayTag EditTag) override;
	//~ IFaerieItemOwnerInterface


	/**------------------------------*/
	/*	  INTERNAL IMPLEMENTATIONS	 */
	/**------------------------------*/
private:
	static FFaerieAddress Encode(FEntryKey Entry, FStackKey Stack);
	static void Decode(FFaerieAddress Address, FEntryKey& Entry, FStackKey& Stack);

	TConstStructView<FInventoryEntry> GetEntryViewImpl(FEntryKey Key) const;

	UInventoryEntryProxy* GetEntryProxyImpl(FEntryKey Key) const;
	UInventoryStackProxy* GetStackProxyImpl(FInventoryKey Key) const;

	// @todo this copies the entry. Kinda wonky, should be used minimally, if at all.
    void GetEntryImpl(FEntryKey Key, FInventoryEntry& Entry) const;

	// Internal implementation for adding items.
	Faerie::Inventory::FEventLog AddStackImpl(const FFaerieItemStack& InStack, bool ForceNewStack);

	// Internal implementations for removing items, specifying an amount.
	Faerie::Inventory::FEventLog RemoveFromEntryImpl(FEntryKey Key, int32 Amount, FFaerieInventoryTag Reason);
	Faerie::Inventory::FEventLog RemoveFromStackImpl(FInventoryKey Key, int32 Amount, FFaerieInventoryTag Reason);

	// FastArray API; used to replicate array changes clientside
	enum EContentChangeType
	{
		StackChange,
		ItemMutation
	};

	void PostContentAdded(const FKeyedInventoryEntry& Entry);
	void PostContentChanged(const FKeyedInventoryEntry& Entry, EContentChangeType ChangeType);
	void PreContentRemoved(const FKeyedInventoryEntry& Entry);

	void BroadcastAddressEvent(EFaerieAddressEventType Type, FFaerieAddress Address);


	/**------------------------------*/
	/*	  STORAGE API - ALL USERS    */
	/**------------------------------*/
public:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent::RegistrationType& GetOnKeyAdded() { return OnKeyAddedCallback; }
	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent::RegistrationType& GetOnKeyUpdated() { return OnKeyUpdatedCallback; }
	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent::RegistrationType& GetOnKeyRemoved() { return OnKeyRemovedCallback; }
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	Faerie::FAddressEvent::RegistrationType& GetOnAddressEvent() { return OnAddressEventCallback; }

	UE_DEPRECATED(5.6, "Direct access to FInventoryEntry is being phased out")
	TConstStructView<FInventoryEntry> GetEntryView(FEntryKey Key) const;

	FFaerieItemStackView GetStackView(FInventoryKey Key) const;

	// Convert an entry key into an array of Inventory Keys.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	TArray<FInventoryKey> GetInvKeysForEntry(FEntryKey Key) const;

	// Gets all entry keys contained in this storage.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Key")
	void GetAllKeys(TArray<FEntryKey>& Keys) const;

	// Retrieve the number of entries in storage.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
    int32 GetStackCount() const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	bool ContainsKey(FEntryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	bool IsValidKey(FInventoryKey Key) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	bool ContainsItem(const UFaerieItem* Item, EFaerieItemEqualsCheck Method) const;

	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	FEntryKey FindItem(const UFaerieItem* Item, EFaerieItemEqualsCheck Method) const;

	// Utility function mainly used with inventories that are expected to only contain a single entry, e.g., pickups.
	UFUNCTION(BlueprintCallable, Category = "Storage|Key")
	FInventoryKey GetFirstKey() const;

	/**
	 * Full version
	 * @return Whether an entry was found.
	 */
	UE_DEPRECATED(5.6, "Direct access to FInventoryEntry is being phased out.")
	UFUNCTION(BlueprintCallable, BlueprintPure = false, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out."))
	bool GetEntry(FEntryKey Key, FInventoryEntry& Entry) const;

	/**
	 * Get the Inventory Entry Proxy representing an entry in this storage.
	 * The proxy will be cached for quick repeat access.
	 * Proxies will be created even if the Key is not valid. This allows the client to prospectively create proxies for
	 * entries that have not replicated yet.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage|Cache")
	UInventoryEntryProxy* GetEntryProxy(FEntryKey Key) const;

	/**
	 * Get the Inventory Stack Proxy representing a stack in this storage.
	 * The proxy will be cached for quick repeat access.
	 * Proxies will be created even if the Key is not valid. This allows the client to prospectively create proxies for
	 * stacks that have not replicated yet.
	 */
	// @todo deprecate in favor of calling Proxy with an Address
	UFUNCTION(BlueprintCallable, Category = "Storage|Cache", DisplayName = "Get Stack Proxy")
	UInventoryStackProxy* GetStackProxy_New(FInventoryKey Key) const;

	UE_DEPRECATED(5.5, "Use Get Stack Proxy instead, and convert to a weak proxy when needed")
	UFUNCTION(BlueprintCallable, Category = "Storage|Cache", meta = (ExpandBoolAsExecs = "ReturnValue"), DisplayName = "Get Stack Proxy (weak)")
	bool GetStackProxy(FInventoryKey Key, FFaerieItemProxy& Proxy);

	/** Get entries en masse */
	UE_DEPRECATED(5.6, "Direct access to FInventoryEntry is being phased out.")
	UFUNCTION(BlueprintCallable, BlueprintPure = false, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FInventoryEntry is being phased out."))
	void GetEntryArray(const TArray<FEntryKey>& Keys, TArray<FInventoryEntry>& Entries) const;

	// Query function to filter for the first matching entry.
	FKeyedInventoryEntry QueryFirst(const Faerie::FStorageFilterFunc& Filter) const;

	// Query function to filter and sort for a subsection of contained entries.
	void QueryAll(const Faerie::FStorageQuery& Query, TArray<FKeyedInventoryEntry>& OutKeys) const;
	void QueryAll(const Faerie::FStorageQuery& Query, TArray<FFaerieAddress>& OutAddresses) const;

	// Query function to filter for the first matching entry.
	UFUNCTION(BlueprintCallable, Category = "Storage|Query")
	FEntryKey QueryFirst(const FBlueprintStorageFilter& Filter) const;

	// Query function to filter and sort for a subsection of contained entries.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Storage|Query")
	void QueryAll(const FFaerieItemStorageBlueprintQuery& Query, TArray<FEntryKey>& OutKeys) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanAddStack(FFaerieItemStackView Stack, EFaerieStorageAddStackBehavior AddStackBehavior) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanAddStacks(const TArray<FFaerieItemStackView>& Stacks, FFaerieExtensionAllowsAdditionArgs Args) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanEditEntry(FEntryKey Key, FFaerieInventoryTag EditTag) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanEditStack(FInventoryKey StackKey, FFaerieInventoryTag EditTag) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanRemoveEntry(FEntryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;

	UFUNCTION(BlueprintCallable, Category = "Storage")
	bool CanRemoveStack(FInventoryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag Reason) const;


	/**---------------------------------*/
	/*	 STORAGE API - AUTHORITY ONLY   */
	/**---------------------------------*/

	// Add a single raw item.
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool AddEntryFromItemObject(UFaerieItem* ItemObject, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add a single raw item.
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool AddItemStack(const FFaerieItemStack& ItemStack, EFaerieStorageAddStackBehavior AddStackBehavior);

	// Add a single raw item, and return the full data about the change.
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly, DisplayName = "Add Item Stack (with Log)")
	FLoggedInventoryEvent AddItemStackWithLog(const FFaerieItemStack& ItemStack, EFaerieStorageAddStackBehavior AddStackBehavior);

	/**
	 * Removes the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool RemoveEntry(FEntryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool RemoveStack(FInventoryKey Key,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes and returns the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool TakeEntry(FEntryKey Key, FFaerieItemStack& OutStack,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Removes and returns the entry with this key if it exists.
	 * An amount of -1 will remove the entire stack.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool TakeStack(FInventoryKey Key, FFaerieItemStack& OutStack,
		UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag, int32 Amount = -1);

	/**
	 * Clear out the entire contents of the storage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
    void Clear(UPARAM(meta = (Categories = "Fae.Inventory.Removal")) FFaerieInventoryTag RemovalTag);

	/**
	 * Add an entry from this storage to another, then remove it from this one, optionally move only part of a stack.
	 * @return The key used by the ToStorage to store the entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	FEntryKey MoveStack(UFaerieItemStorage* ToStorage, FInventoryKey Key, int32 Amount = -1,
		EFaerieStorageAddStackBehavior AddStackBehavior = EFaerieStorageAddStackBehavior::AddToAnyStack);

	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	FEntryKey MoveEntry(UFaerieItemStorage* ToStorage, FEntryKey Key, EFaerieStorageAddStackBehavior AddStackBehavior);

	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool MergeStacks(FEntryKey Entry, FStackKey FromStack, FStackKey ToStack, int32 Amount = -1);

	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	bool SplitStack(FEntryKey Entry, FStackKey Stack, int32 Amount);

	/** Call MoveEntry on all entries in this storage. */
	UFUNCTION(BlueprintCallable, Category = "Storage", BlueprintAuthorityOnly)
	void Dump(UFaerieItemStorage* ToStorage);


	/**-------------*/
	/*	 DELEGATES	*/
	/**-------------*/
protected:
	Faerie::FAddressEvent OnAddressEventCallback;

	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent OnKeyAddedCallback;
	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent OnKeyUpdatedCallback;
	UE_DEPRECATED(5.6, "Use Address version")
	Faerie::FEntryKeyEvent OnKeyRemovedCallback;

	// Broadcast whenever an entry is added, or a stack amount is increased.
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FFaerieAddressEvent OnAddressEvent;

	// Broadcast whenever an entry is added, or a stack amount is increased.
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FEntryKeyEvent OnKeyAdded;

	// Broadcast whenever data for a key is changed.
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FEntryKeyEvent OnKeyUpdated;

	// Broadcast whenever an entry is removed entirely, or a stack amount is decreased.
	UPROPERTY(BlueprintAssignable, Transient, Category = "Events")
	FEntryKeyEvent OnKeyRemoved;


	/**-------------*/
	/*	 VARIABLES	*/
	/**-------------*/
private:
	// The internal map containing the contents of the storage.
	UPROPERTY(Replicated)
	FInventoryContent EntryMap;

	// These properties are transient, mainly so that editor code that accesses them doesn't need to worry about Caches
	// being left around. Using weak pointers here is intentional. We don't want this storage to keep these alive. They
	// should be stored in a strong pointer by whatever requested them, and once nothing needs the proxies, they will die.
	// Effectively mutable, as these are written to by the const proxy accessors.
	// @todo how will these maps get cleaned up, to they don't accrue hundreds of stale ptrs?

	// Locally stored proxies per entry.
	UPROPERTY(Transient)
	TMap<FEntryKey, TWeakObjectPtr<UInventoryEntryProxy>> LocalEntryProxies;

	// Locally stored proxies per individual stack.
	UPROPERTY(Transient)
	TMap<FInventoryKey, TWeakObjectPtr<UInventoryStackProxy>> LocalStackProxies;
};