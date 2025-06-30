// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemAssetPreviewScene.h"
#include "ThumbnailHelpers.h"

class IFaerieItemDataProxy;
class AItemRepresentationActor;
class UBoxComponent;
class UFaerieItemAsset;
class UFaerieItemMeshLoader;
class UFaerieItemMeshComponent;

namespace Faerie::Ed
{
	class FItemAssetPreviewSceneThumbnail final : public FThumbnailPreviewScene
	{
	public:
		FItemAssetPreviewSceneThumbnail(const IFaerieItemDataProxy* Proxy);
		virtual ~FItemAssetPreviewSceneThumbnail() override = default;

		//~ FAdvancedPreviewScene
		virtual void Tick(float InDeltaTime) override;
		//~ FAdvancedPreviewScene

		virtual void GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin, float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const override;

		FBoxSphereBounds GetBounds() const;

		void SetItemProxy(const IFaerieItemDataProxy* Proxy);

		void RefreshMesh();

	private:
		FItemPreviewSceneData SceneData;
	};
}