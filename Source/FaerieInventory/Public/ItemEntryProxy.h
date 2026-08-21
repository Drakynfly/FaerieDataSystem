// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxyBase.h"
#include "ItemEntryProxy.generated.h"

class UFaerieItemStorage;

/*
 * Class for a proxy to an entry in a UFaerieItemStorage.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"), BlueprintType, Within = FaerieItemStorage)
class UFaerieItemEntryProxy final : public UObject, public IFaerieItemDataProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ UObject
	virtual UWorld* GetWorld() const override;
	//~ UObject

	//~ Faerie::ItemData::IViewBase
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override;
	virtual IFaerieItemOwnerInterface* GetItemOwner() const override;
	//~ Faerie::ItemData::IViewBase

	//~ IFaerieItemDataProxy
	UE_REWRITE virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override { return OnProxyEvent; }
	//~ IFaerieItemDataProxy

	FAERIEINVENTORY_API UE_REWRITE int32 GetItemVersion() const { return LocalItemVersion; }
	FAERIEINVENTORY_API UE_REWRITE FFaerieEntryKey GetKey() const { return Key; }

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EntryProxy")
	UFaerieItemStorage* GetItemStorage() const;

	void NotifyLocalCreation();
	void NotifyDelayedCreation();
	void NotifyUpdate();
	void NotifyRemoval();

	// EntryProxies do not generate ItemDataChanged events as they are not created for mutable instances.
	//void NotifyItemDataChanged(FGameplayTag EditTag);

	bool VerifyStatus() const;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "EntryProxy")
	FFaerieEntryKey Key;

	// Tracks the item version locally, so the client can track item state.
	// -1 means that this Entry has never received a NotifyCreation and is not-yet-valid or invalid.
	// 0 means that this Entry has received a NotifyCreation, but no NotifyUpdate.
	// Numbers greater increment the Updates we have received.
	// This number is not guaranteed to match between server and client, or between clients. It is purely the record of
	// how many times a machine has received a new version.
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "EntryProxy")
	int32 LocalItemVersion = -1;

private:
	Faerie::ItemData::FProxyChangeEvent OnProxyEvent;
};