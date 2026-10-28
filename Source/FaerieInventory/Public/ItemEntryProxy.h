// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxyBase.h"
#include "ViewModels/FaerieProxyViewModel.h"
#include "ItemEntryProxy.generated.h"

class UFaerieItemStorage;

/*
 * Class for a proxy to an entry in a UFaerieItemStorage.
 */
UCLASS(meta = (DontUseGenericSpawnObject = "true"), BlueprintType, Within = FaerieItemStorage)
class UFaerieItemEntryProxy final : public UFaerieProxyViewModelBase, public IFaerieItemDataProxy
{
	GENERATED_BODY()

	friend UFaerieItemStorage;

public:
	//~ Faerie::ItemData::IViewBase
#if WITH_EDITOR
	// As IFaerieContainerProxy is only implemented by runtime containers, it will never provide an item asset object.
	virtual const UFaerieItem* GetItemAsset() const override { return nullptr; }
#endif
	virtual FMassEntityHandle GetEntityHandle() const override;
	virtual int32 GetCopies() const override;
	//~ Faerie::ItemData::IViewBase

	UE_REWRITE FAERIEINVENTORY_API int32 GetItemVersion() const { return LocalItemVersion; }
	UE_REWRITE FAERIEINVENTORY_API FFaerieEntryKey GetKey() const { return Key; }

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EntryProxy")
	UFaerieItemStorage* GetItemStorage() const;

	void NotifyLocalCreation();
	void NotifyDelayedCreation();
	void NotifyUpdate();
	void NotifyRemoval();

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
};