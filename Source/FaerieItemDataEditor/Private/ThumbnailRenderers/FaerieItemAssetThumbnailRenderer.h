// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "ThumbnailRendering/DefaultSizedThumbnailRenderer.h"
#include "FaerieItemAssetThumbnailRenderer.generated.h"

class UFaerieItemAsset;

namespace Faerie::Ed
{
	class FItemAssetPreviewSceneThumbnail;
}

/**
 * 
 */
UCLASS()
class FAERIEITEMDATAEDITOR_API UFaerieItemAssetThumbnailRenderer : public UDefaultSizedThumbnailRenderer, public IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	//~ Begin UObject
	virtual void BeginDestroy() override;
	//~ End UObject

	//~ Begin UThumbnailRenderer
	virtual bool CanVisualizeAsset(UObject* Object) override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;
	virtual EThumbnailRenderFrequency GetThumbnailRenderFrequency(UObject* Object) const override;
	//~ End UThumbnailRenderer

	//~ Begin IFaerieItemDataProxy
	virtual const UFaerieItem* GetItemObject() const override;
	virtual int32 GetCopies() const override { return 1; }
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const override;
	virtual UThumbnailInfo* GetThumbnailInfo() const override;
	//~ End IFaerieItemDataProxy

private:
	UPROPERTY()
	TObjectPtr<UFaerieItemAsset> ItemAsset;

	Faerie::Ed::FItemAssetPreviewSceneThumbnail* ThumbnailScene;
};
