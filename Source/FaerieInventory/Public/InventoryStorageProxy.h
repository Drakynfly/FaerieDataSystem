// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "InventoryDataStructs.h"
#include "StructUtils/StructView.h"
#include "InventoryStorageProxy.generated.h"

class UInventoryEntryStorageProxy;
using FEntryStorageProxyEvent = TMulticastDelegate<void(UInventoryEntryStorageProxy*)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCacheEvent, UInventoryEntryStorageProxy*, Proxy);

/*
 * Base class for proxies to an FInventoryEntry struct. For all intents and purposes these only live in a UFaerieItemStorage,
 * so in almost every case, the child UInventoryEntryStorageProxy is what you want.
 */
UCLASS(Abstract, BlueprintType)
class UInventoryEntryProxyBase : public UObject, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	/** Get all stacks for this item. */
	UE_DEPRECATED(5.6, "Direct access to FKeyedStacks is being phased out")
	UFUNCTION(BlueprintCallable, meta = (DeprecatedFunction, DeprecationMessage = "Direct access to FKeyedStacks is being phased out"))
	FAERIEINVENTORY_API TArray<FKeyedStack> GetAllStacks() const;

	/** Get the stack limit of this item. */
	UFUNCTION(BlueprintCallable, Category = "Entry Cache")
	FAERIEINVENTORY_API int32 GetStackLimit() const;

protected:
	UE_DEPRECATED(5.6, "Direct access to FInventoryEntry is being phased out")
	virtual TConstStructView<FInventoryEntry> GetInventoryEntry() const PURE_VIRTUAL(UInventoryEntryProxyBase::GetInventoryEntry, return TConstStructView<FInventoryEntry>(); )
};

/*
 * Base class for a proxy to an FInventoryEntry in a UFaerieItemStorage.
 * Proxies can be created predictively. When this is the case, ItemVersion will equal -1.
 */
UCLASS(Abstract)
class UInventoryEntryStorageProxy : public UInventoryEntryProxyBase
{
	GENERATED_BODY()

public:
	//~ IFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	//~ IFaerieItemDataProxy

	//~ UInventoryEntryProxyBase
	virtual TConstStructView<FInventoryEntry> GetInventoryEntry() const override final;
	//~ UInventoryEntryProxyBase

	FAERIEINVENTORY_API UFaerieItemStorage* GetStorage() const { return ItemStorage.Get(); }
	FAERIEINVENTORY_API int32 GetItemVersion() const { return LocalItemVersion; }
	FAERIEINVENTORY_API virtual FEntryKey GetKey() const PURE_VIRTUAL(UInventoryEntryStorageProxy::GetKey, return FEntryKey(); )

	FAERIEINVENTORY_API FEntryStorageProxyEvent::RegistrationType& GetOnCacheUpdated() { return OnCacheUpdatedNative; }
	FAERIEINVENTORY_API FEntryStorageProxyEvent::RegistrationType& GetOnCacheRemoved() { return OnCacheRemovedNative; }

protected:
	void NotifyCreation();
	void NotifyUpdate();
	void NotifyRemoval();

	bool VerifyStatus() const;

	// Broadcast when this proxy is first initialized, or receives an update.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FCacheEvent OnCacheUpdated;

	// Broadcast when the entry represented by this proxy is being partially removed or deleted.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FCacheEvent OnCacheRemoved;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

	// Tracks the item version locally, so the client can track item state.
	// -1 means that this Entry has never received a NotifyCreation and is not-yet-valid or invalid.
	// 0 means that this Entry has received a NotifyCreation, but no NotifyUpdate.
	// Numbers greater increment the Updates we have received.
	// This number is not guaranteed to match between server and client, or between clients. It is purely the record of
	// how many times a machine has received a new version.
	UPROPERTY(BlueprintReadOnly, Category = "Entry Cache")
	int32 LocalItemVersion = -1;

private:
	FEntryStorageProxyEvent OnCacheUpdatedNative;
	FEntryStorageProxyEvent OnCacheRemovedNative;
};

class UFaerieItemStorage;

/**
 * An implementation of UInventoryEntryStorageProxy that reads from an inventory entry inside an item storage.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"))
class FAERIEINVENTORY_API UInventoryEntryProxy : public UInventoryEntryStorageProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ UInventoryEntryStorageProxy
	virtual FEntryKey GetKey() const override;
	//~ UInventoryEntryStorageProxy

protected:
	UPROPERTY(BlueprintReadOnly, Category = "EntryProxy")
	FEntryKey Key;
};

/**
 * An implementation of UInventoryEntryStorageProxy that reads from an inventory stack inside an item storage.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"))
class FAERIEINVENTORY_API UInventoryStackProxy : public UInventoryEntryStorageProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ IFaerieItemDataProxy
	virtual int32 GetCopies() const override;
	//~ IFaerieItemDataProxy

	//~ UInventoryEntryStorageProxy
	virtual FEntryKey GetKey() const override;
	//~ UInventoryEntryStorageProxy

	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	FInventoryKeyHandle GetHandle() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	FFaerieAddressableHandle GetAddressable() const;

protected:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "StackProxy")
	FInventoryKey Key;
};