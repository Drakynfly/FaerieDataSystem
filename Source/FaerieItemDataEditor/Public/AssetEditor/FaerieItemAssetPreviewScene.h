// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "AdvancedPreviewScene.h"
#include "FaerieItemAssetEditorCustomSettings.h"
#include "GameplayTagContainer.h"

class IFaerieItemDataProxy;
class AItemRepresentationActor;
class UBoxComponent;
class UFaerieItemMeshLoader;
class UFaerieItemMeshComponent;

namespace Faerie::Ed
{
	struct FItemPreviewSceneData
	{
		FItemPreviewSceneData(FPreviewScene* Scene);
		~FItemPreviewSceneData();

		void InitializeScene();

		void SetProxy(const IFaerieItemDataProxy* Proxy);

		void SetShowBounds(bool InShowBounds);
		void RefreshItemData();

		FBoxSphereBounds GetBounds() const;

	//private:
		void OnDisplayFinished(bool Success);

		bool ShowBounds = false;

		FPreviewScene* Scene = nullptr;

		// Cube mesh that is shown when no other mesh is found to obviously report an invalid mesh visually.
		TObjectPtr<UStaticMeshComponent> DefaultCube;

		// Box component to visualize capacity token bounds.
		TObjectPtr<UBoxComponent> BoundsBox;

		// The visual representation of the item, if a token specifies using an actor.
		TObjectPtr<AItemRepresentationActor> ItemActor;

		// Component used to when we don't create an ItemActor. Always valid, but will have no data when using ItemActor.
		TObjectPtr<UFaerieItemMeshComponent> ItemMeshComponent;

		FGameplayTag MeshPurposeTag;

		const IFaerieItemDataProxy* ItemProxy = nullptr;

		bool CenterMeshByBounds = true;
	};

	/**
	 *
	 */
	class FItemDataProxyPreviewScene final : public FAdvancedPreviewScene
	{
	public:
		FItemDataProxyPreviewScene(ConstructionValues CVS);
		virtual ~FItemDataProxyPreviewScene() override = default;

		//~ FAdvancedPreviewScene
		virtual void Tick(float InDeltaTime) override;
		//~ FAdvancedPreviewScene

		void SetSettings(UFaerieItemAssetEditorCustomSettings* Settings);
		void SyncSettings();

		FBoxSphereBounds GetBounds() const;

		void SetItemProxy(const IFaerieItemDataProxy* Proxy);

		void RefreshMesh();

	private:
		FItemPreviewSceneData SceneData;

		TObjectPtr<UFaerieItemAssetEditorCustomSettings> EditorSettings;
	};
}