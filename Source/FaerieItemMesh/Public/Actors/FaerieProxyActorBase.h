// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "FaerieItemVisualBase.h"
#include "FaerieProxyActorBase.generated.h"

/**
 * The base class for actors that visualize an item proxy struct.
 */
UCLASS(Abstract)
class FAERIEITEMMESH_API AFaerieProxyActorBase : public AFaerieItemVisualBase
{
	GENERATED_BODY()

public:
	AFaerieProxyActorBase();

	//~ UFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override;
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	//~ UFaerieItemDataProxy

	FFaerieItemProxy GetSourceProxy() const { return DataSource; }

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemRepresentationActor")
	void SetSourceProxy(FFaerieItemProxy Source);

protected:
	// The wrapper for the data we are going to display. By keeping the data abstracted behind a FaerieItemProxy,
	// this allows AFaerieProxyActorBase to display data from an Inventory, or an Equipment, etc., just as well,
	// with the same API.
	// Proxies typically cannot replicate. If a particular child wants to replicate some or all of the data, it
	// needs to extract out the data it needs into a separate replicated variable.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FFaerieItemProxy DataSource = nullptr;
};