// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "WidgetPreview.h"
#include "FaerieWidgetPreview.generated.h"

class UFaerieItemAsset;

/**
 * 
 */
UCLASS()
class FAERIEITEMDATAEDITOR_API UFaerieWidgetPreview : public UWidgetPreview, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	//~ IFaerieItemDataProxy
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override { return 1; }
	virtual IFaerieItemOwnerInterface* GetItemOwner() const override;
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() override;
	//~ IFaerieItemDataProxy

	void InitFaerieWidgetPreview(UFaerieItemAsset* InAsset);

protected:
	UPROPERTY()
	TWeakObjectPtr<UFaerieItemAsset> Asset;

	// Unused change event for Proxy API. Could use in the future to push updates to the PreviewWidget without rebuilding???
	Faerie::ItemData::FProxyChangeEvent OnChangeEvent;
};
