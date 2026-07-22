// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemAssetPreviewScene.h"
#include "ThumbnailHelpers.h"

class IFaerieItemDataProxy;
class AFaerieProxyActorBase;
class UBoxComponent;
class UFaerieItemAsset;
class UFaerieItemMeshLoader;
class UFaerieItemMeshComponent;

namespace Faerie::Editor
{
	class FItemAssetPreviewSceneThumbnail final : public FThumbnailPreviewScene
	{
	public:
		FItemAssetPreviewSceneThumbnail(const FFaerieItemProxy& Proxy);
		virtual ~FItemAssetPreviewSceneThumbnail() override = default;

		//~ FAdvancedPreviewScene
		virtual void Tick(float InDeltaTime) override;
		//~ FAdvancedPreviewScene

	protected:
		virtual void GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const override;

	public:
		FBoxSphereBounds GetBounds() const;

		void SetItemProxy(const FFaerieItemProxy& Proxy);

		void RefreshMesh();

	private:
		FItemPreviewSceneData SceneData;
	};
}