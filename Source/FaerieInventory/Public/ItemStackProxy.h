// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxyBase.h"
#include "ViewModels/FaerieProxyViewModel.h"
#include "ItemStackProxy.generated.h"

class UFaerieItemStorage;

/*
 * Class for a proxy to an address in a UFaerieItemStorage.
 * Proxies can be created predictively. When this is the case, ItemVersion will equal -1.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"), BlueprintType, Within = FaerieItemStorage)
class UFaerieItemStackProxy final : public UFaerieProxyViewModelBase, public IFaerieContainerProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ Faerie::ItemData::IViewBase
	virtual FMassEntityHandle GetEntityHandle() const override;
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override;
	//~ Faerie::ItemData::IViewBase

	//~ IFaerieContainerProxy
	UE_REWRITE virtual FFaerieAddress Proxy_GetAddress() const override { return Address; }
	UE_REWRITE virtual FFaerieItemNetworkHandle Proxy_GetNetworkHandle() const override { return GetNetworkHandle(); }
	//~ IFaerieContainerProxy

	UE_REWRITE FFaerieAddress GetAddress() const { return Address; }

	UE_REWRITE FAERIEINVENTORY_API int32 GetItemVersion() const { return LocalItemVersion; }
	FAERIEINVENTORY_API FFaerieEntryKey GetKey() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	FFaerieItemNetworkHandle GetNetworkHandle() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	UFaerieItemStorage* GetItemStorage() const;

	void NotifyLocalCreation();
	void NotifyDelayedCreation();
	void NotifyUpdate();
	void NotifyRemoval();

	bool VerifyStatus() const;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "StackProxy")
	FFaerieAddress Address;

	// Tracks the item version locally, so the client can track item state.
	// -1 means that this Entry has never received a NotifyCreation and is not-yet-valid or invalid.
	// 0 means that this Entry has received a NotifyCreation, but no NotifyUpdate.
	// Numbers greater increment the Updates we have received.
	// This number is not guaranteed to match between server and client, or between clients. It is purely the record of
	// how many times a machine has received a new version.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "StackProxy")
	int32 LocalItemVersion = -1;
};