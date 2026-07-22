// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieInventoryTag.h"
#include "FaerieItemProxyBase.h"
#include "TypedGameplayTags.h"
#include "ItemStackProxy.generated.h"

class UFaerieItemStorage;

namespace Faerie::Inventory
{
	FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, ProxyCreated)
	FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, ProxyUpdated)
	FAERIEINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_TYPED_EXTERN(FFaerieInventoryTag, ProxyRemoved)
}

/*
 * Class for a proxy to an address in a UFaerieItemStorage.
 * Proxies can be created predictively. When this is the case, ItemVersion will equal -1.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"), BlueprintType)
class UFaerieItemStackProxy final : public UObject, public IFaerieContainerProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ UObject
	virtual UWorld* GetWorld() const override;
	//~ UObject

	//~ IFaerieItemDataProxy
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override;
	virtual IFaerieItemOwnerInterface* GetItemOwner() const override;
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override { return OnProxyEvent; }
	//~ IFaerieItemDataProxy

	//~ IFaerieContainerProxy
	UE_REWRITE virtual FFaerieAddress Proxy_GetAddress() const override { return Address; }
	virtual FFaerieItemNetworkHandle Proxy_GetNetworkHandle() const override;
	//~ IFaerieContainerProxy

	UE_REWRITE FFaerieAddress GetAddress() const { return Address; }

	FAERIEINVENTORY_API UE_REWRITE UFaerieItemStorage* GetStorage() const { return ItemStorage.Get(); }
	FAERIEINVENTORY_API UE_REWRITE int32 GetItemVersion() const { return LocalItemVersion; }
	FAERIEINVENTORY_API FFaerieEntryKey GetKey() const;

	UFUNCTION(BlueprintCallable, Category = "Faerie|StackProxy")
	FFaerieItemNetworkHandle GetNetworkHandle() const;

protected:
	void NotifyCreation();
	void NotifyUpdate();
	void NotifyRemoval();
	void NotifyItemDataChanged(FGameplayTag EditTag);

	bool VerifyStatus() const;

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
	Faerie::ItemData::FProxyChangeEvent OnProxyEvent;
};