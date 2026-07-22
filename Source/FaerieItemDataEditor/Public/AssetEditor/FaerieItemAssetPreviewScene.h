// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "AdvancedPreviewScene.h"
#include "FaerieItemAssetEditorCustomSettings.h"
#include "FaerieItemProxy.h"
#include "GameplayTagContainer.h"

class AFaerieProxyActorBase;
class IFaerieItemDataProxy;
class UBoxComponent;
class UFaerieItemMeshLoader;
class UFaerieItemMeshComponent;

namespace Faerie::Editor
{
	struct FItemPreviewSceneData
	{
		FItemPreviewSceneData(FPreviewScene* Scene);
		~FItemPreviewSceneData();

		void InitializeScene();

		void SetProxy(const FFaerieItemProxy& Proxy);

		void SetShowBounds(bool InShowBounds);
		void SetMeshPurposeTag(FGameplayTag Tag);

		void RefreshItemData();

		FBoxSphereBounds GetBounds() const;

	//private:
		void OnDisplayFinished(bool Success);

		bool ShowBounds = false;

		FPreviewScene* Scene = nullptr;

		// Cube mesh that is shown when no other mesh is found to obviously report an invalid mesh visually.
		TWeakObjectPtr<UStaticMeshComponent> DefaultCube;

		// Box component to visualize capacity fragment bounds.
		TWeakObjectPtr<UBoxComponent> BoundsBox;

		// The visual representation of the item, if a fragment specifies using an actor.
		TWeakObjectPtr<AFaerieProxyActorBase> ItemActor;

		// Component used to when we don't create an ItemActor. Always valid, but will have no data when using ItemActor.
		TWeakObjectPtr<UFaerieItemMeshComponent> ItemMeshComponent;

		FGameplayTag MeshPurposeTag;

		FFaerieItemProxy ItemProxy;

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

		void SetItemProxy(const FFaerieItemProxy& Proxy);

		void RefreshMesh();

	private:
		FItemPreviewSceneData SceneData;

		TObjectPtr<UFaerieItemAssetEditorCustomSettings> EditorSettings;
	};
}