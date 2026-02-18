// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieItemProxy.h"
#include "InventoryStorageProxy.generated.h"

class UFaerieItemStorage;
class UInventoryStackProxy;

namespace Faerie::Inventory
{
	enum EStackProxyEventType
	{
		Updated,
		Removed
	};
	using FStackProxyEvent = TMulticastDelegate<void(UInventoryStackProxy*, EStackProxyEventType)>;
}
using FEntryStorageProxyEvent = TMulticastDelegate<void(UInventoryStackProxy*)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFaerieStackProxyEvent, UInventoryStackProxy*, Proxy);

/*
 * Class for a proxy to an address in a UFaerieItemStorage.
 * Proxies can be created predictively. When this is the case, ItemVersion will equal -1.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"), BlueprintType)
class UInventoryStackProxy : public UObject, public IFaerieItemDataProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ IFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	virtual FFaerieItemStack Release(int32 Copies) const override;
	//~ IFaerieItemDataProxy

	FAERIEINVENTORY_API UFaerieItemStorage* GetStorage() const { return ItemStorage.Get(); }
	FAERIEINVENTORY_API int32 GetItemVersion() const { return LocalItemVersion; }
	FAERIEINVENTORY_API FEntryKey GetKey() const;

	FAERIEINVENTORY_API Faerie::Inventory::FStackProxyEvent::RegistrationType& GetOnProxyEvent() { return OnProxyEvent; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	FFaerieAddressableHandle GetAddressable() const;

	/** Get the stack limit of this item. */
	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	int32 GetStackLimit() const;

protected:
	void NotifyCreation();
	void NotifyUpdate();
	void NotifyRemoval();

	bool VerifyStatus() const;

	// Broadcast when this proxy is first initialized, or receives an update.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FFaerieStackProxyEvent OnCacheUpdated;

	// Broadcast when the entry represented by this proxy is being partially removed or deleted.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FFaerieStackProxyEvent OnCacheRemoved;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "StackProxy")
	TWeakObjectPtr<UFaerieItemStorage> ItemStorage;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "StackProxy")
	FFaerieAddress Address;

	// Tracks the item version locally, so the client can track item state.
	// -1 means that this Entry has never received a NotifyCreation and is not-yet-valid or invalid.
	// 0 means that this Entry has received a NotifyCreation, but no NotifyUpdate.
	// Numbers greater increment the Updates we have received.
	// This number is not guaranteed to match between server and client, or between clients. It is purely the record of
	// how many times a machine has received a new version.
	UPROPERTY(BlueprintReadOnly, Category = "StackProxy")
	int32 LocalItemVersion = -1;

private:
	Faerie::Inventory::FStackProxyEvent OnProxyEvent;
};