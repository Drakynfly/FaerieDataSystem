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
	//~ Faerie::ItemData::IViewBase
	virtual FMassEntityHandle GetEntityHandle() const override;
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const override;
	virtual int32 GetCopies() const override { return 1; }
	virtual UObject* GetItemOwner() const override;
	//~ Faerie::ItemData::IViewBase

	void InitFaerieWidgetPreview(UFaerieItemAsset* InAsset);

protected:
	UPROPERTY()
	TWeakObjectPtr<UFaerieItemAsset> Asset;
};
