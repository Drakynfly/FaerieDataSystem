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

	//~ Faerie::ItemData::IViewBase
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override;
	virtual const IFaerieItemOwnerInterface* GetItemOwner() const override;
	//~ Faerie::ItemData::IViewBase

	//~ IFaerieItemDataProxy
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override;
	//~ IFaerieItemDataProxy

	const FFaerieItemProxy& GetSourceProxy() const { return DataSource; }

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemRepresentationActor", meta = (AutoCreateRefTerm = "Source"))
	void SetSourceProxy(const FFaerieItemProxy& Source);

protected:
	// The wrapper for the data we are going to display. By keeping the data abstracted behind a FaerieItemProxy,
	// this allows AFaerieProxyActorBase to display data from an Inventory, or an Equipment, etc., just as well,
	// with the same API.
	// Proxies typically cannot replicate. If a particular child wants to replicate some or all of the data, it
	// needs to extract out the data it needs into a separate replicated variable.
	UPROPERTY(BlueprintReadOnly, Category = "State")
	FFaerieItemProxy DataSource;
};