// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetThumbnailScene.h"
#include "Editor.h"
#include "FaerieItemProxy.h"
#include "ThumbnailRendering/SceneThumbnailInfo.h"

namespace Faerie::Ed
{
	FItemAssetPreviewSceneThumbnail::FItemAssetPreviewSceneThumbnail(const IFaerieItemDataProxy* Proxy)
		: SceneData(this)
	{
		SceneData.ItemProxy = Proxy;

		SceneData.InitializeScene();
	}

	void FItemAssetPreviewSceneThumbnail::Tick(const float InDeltaTime)
	{
		FThumbnailPreviewScene::Tick(InDeltaTime);

		if (GEditor->bIsSimulatingInEditor ||
			GEditor->PlayWorld != nullptr)
		{
			return;
		}

		GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}

	void FItemAssetPreviewSceneThumbnail::GetViewMatrixParameters(const float InFOVDegrees, FVector& OutOrigin,
		float& OutOrbitPitch, float& OutOrbitYaw, float& OutOrbitZoom) const
	{
		const FBoxSphereBounds Bounds = GetBounds();

		const float HalfFOVRadians = FMath::DegreesToRadians<float>(InFOVDegrees) * 0.5f;
		// Add extra size to view slightly outside of the sphere to compensate for perspective

		const float HalfMeshSize = static_cast<float>(Bounds.SphereRadius * 1.15);
		const float BoundsZOffset = GetBoundsZOffset(Bounds);
		const float TargetDistance = HalfMeshSize / FMath::Tan(HalfFOVRadians);

		USceneThumbnailInfo* ThumbnailInfo = Cast<USceneThumbnailInfo>(SceneData.ItemProxy->GetThumbnailInfo());
		if (IsValid(ThumbnailInfo))
		{
			if (TargetDistance + ThumbnailInfo->OrbitZoom < 0)
			{
				ThumbnailInfo->OrbitZoom = -TargetDistance;
			}
		}
		else
		{
			ThumbnailInfo = USceneThumbnailInfo::StaticClass()->GetDefaultObject<USceneThumbnailInfo>();
		}

		OutOrigin = FVector(0, 0, -BoundsZOffset);
		OutOrbitPitch = ThumbnailInfo->OrbitPitch;
		OutOrbitYaw = ThumbnailInfo->OrbitYaw;
		OutOrbitZoom = TargetDistance + ThumbnailInfo->OrbitZoom;
	}

	FBoxSphereBounds FItemAssetPreviewSceneThumbnail::GetBounds() const
	{
		return SceneData.GetBounds();
	}

	void FItemAssetPreviewSceneThumbnail::SetItemProxy(const IFaerieItemDataProxy* Proxy)
	{
		SceneData.SetProxy(Proxy);
	}

	void FItemAssetPreviewSceneThumbnail::RefreshMesh()
	{
		SceneData.RefreshItemData();
	}
}
