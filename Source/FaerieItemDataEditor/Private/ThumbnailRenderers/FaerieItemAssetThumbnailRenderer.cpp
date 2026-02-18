// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemAssetThumbnailRenderer.h"
#include "FaerieItemAsset.h"
#include "SceneView.h"
#include "AssetEditor/FaerieItemAssetThumbnailScene.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemAssetThumbnailRenderer)

#define LOCTEXT_NAMESPACE "FaerieItemAssetThumbnailRenderer"

void UFaerieItemAssetThumbnailRenderer::BeginDestroy()
{
	if (ThumbnailScene != nullptr)
	{
		delete ThumbnailScene;
		ThumbnailScene = nullptr;
	}

	Super::BeginDestroy();
}

bool UFaerieItemAssetThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return IsValid(Cast<UFaerieItemAsset>(Object));
}

void UFaerieItemAssetThumbnailRenderer::Draw(UObject* Object, const int32 X, const int32 Y, const uint32 Width, const uint32 Height,
											 FRenderTarget* RenderTarget, FCanvas* Canvas, const bool bAdditionalViewFamily)
{
	ItemAsset = Cast<UFaerieItemAsset>(Object);

	if (IsValid(ItemAsset))
	{
		if (!ThumbnailScene)
		{
			ThumbnailScene = new Faerie::Editor::FItemAssetPreviewSceneThumbnail(this);
		}

		ThumbnailScene->SetItemProxy(this);
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(RenderTarget, ThumbnailScene->GetScene(), FEngineShowFlags(ESFIM_Game))
			.SetTime(UThumbnailRenderer::GetTime())
			.SetAdditionalViewFamily(bAdditionalViewFamily));

		ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
		ViewFamily.EngineShowFlags.MotionBlur = 0;
		ViewFamily.EngineShowFlags.LOD = 0;

		RenderViewFamily(Canvas, &ViewFamily, ThumbnailScene->CreateView(&ViewFamily, X, Y, Width, Height));
		ThumbnailScene->SetItemProxy(nullptr);
	}
}

EThumbnailRenderFrequency UFaerieItemAssetThumbnailRenderer::GetThumbnailRenderFrequency(UObject* Object) const
{
	// @todo disable RealTime if we don't have a mesh...

	return EThumbnailRenderFrequency::Realtime;
}

const UFaerieItem* UFaerieItemAssetThumbnailRenderer::GetItemObject() const
{
	if (IsValid(ItemAsset))
	{
		return ItemAsset->GetEditorItemView();
	}
	return nullptr;
}

TScriptInterface<IFaerieItemOwnerInterface> UFaerieItemAssetThumbnailRenderer::GetItemOwner() const
{
	return nullptr;
}

UThumbnailInfo* UFaerieItemAssetThumbnailRenderer::GetThumbnailInfo() const
{
	if (IsValid(ItemAsset))
	{
		return ItemAsset->ThumbnailInfo;
	}
	return nullptr;
}
