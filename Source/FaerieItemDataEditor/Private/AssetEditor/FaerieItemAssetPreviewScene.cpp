// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "Editor.h"
#include "FaerieItem.h"

#include "Actors/FaerieProxyActorBase.h"
#include "Components/BoxComponent.h"
#include "Components/FaerieItemMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "GameFramework/WorldSettings.h"

#include "Tokens/FaerieCapacityToken.h"
#include "Tokens/FaerieMeshToken.h"
#include "Tokens/FaerieVisualActorClassToken.h"

namespace Faerie::Ed
{
	FItemPreviewSceneData::FItemPreviewSceneData(FPreviewScene* Scene)
		: Scene(Scene)
	{
		MeshPurposeTag = ItemMesh::Tags::MeshPurpose_Default;
	}

	FItemPreviewSceneData::~FItemPreviewSceneData()
	{
		if (IsValid(ItemActor))
		{
			ItemActor->Destroy();
			ItemActor = nullptr;
		}

		if (IsValid(ItemMeshComponent) && !ItemMeshComponent->HasAnyFlags(RF_BeginDestroyed))
		{
			ItemMeshComponent->ClearItemMesh();
			ItemMeshComponent->DestroyComponent();
			ItemMeshComponent = nullptr;
		}
	}

	void FItemPreviewSceneData::InitializeScene()
	{
		auto World = Scene->GetWorld();

		{
			// Disable killing actors outside of the world
			AWorldSettings* WorldSettings = World->GetWorldSettings(true);
			WorldSettings->bEnableWorldBoundsChecks = false;
		}

		{
			UStaticMesh* PreviewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Cube.Cube"), nullptr, LOAD_None, nullptr);

			DefaultCube = NewObject<UStaticMeshComponent>(GetTransientPackage());
			DefaultCube->SetStaticMesh(PreviewMesh);
			DefaultCube->SetVisibility(false);
			DefaultCube->bSelectable = true;

			Scene->AddComponent(DefaultCube, FTransform::Identity);
		}

		{
			BoundsBox = NewObject<UBoxComponent>(GetTransientPackage());
			BoundsBox->SetLineThickness(0.2f);
			BoundsBox->SetVisibility(false);

			Scene->AddComponent(BoundsBox, FTransform::Identity);
		}

		{
			ItemMeshComponent = NewObject<UFaerieItemMeshComponent>(GetTransientPackage());
			ItemMeshComponent->CenterMeshByBounds = CenterMeshByBounds;
			ItemMeshComponent->CacheSkeletalBoundsInPose = true;
			ItemMeshComponent->PreferredTag = MeshPurposeTag;
			//ItemMeshComponent->bSelectable = true;

			Scene->AddComponent(ItemMeshComponent, FTransform::Identity);
		}
	}

	void FItemPreviewSceneData::SetProxy(const IFaerieItemDataProxy* Proxy)
	{
		ItemProxy = Proxy;
		RefreshItemData();
	}

	void FItemPreviewSceneData::SetShowBounds(const bool InShowBounds)
	{
		ShowBounds = InShowBounds;
		if (IsValid(BoundsBox))
		{
			BoundsBox->SetVisibility(ShowBounds);
		}
	}

	void FItemPreviewSceneData::SetMeshPurposeTag(const FGameplayTag Tag)
	{
		MeshPurposeTag = Tag;
		if (IsValid(ItemMeshComponent))
		{
			ItemMeshComponent->SetPreferredTag(MeshPurposeTag);
		}
	}

	void FItemPreviewSceneData::RefreshItemData()
	{
		if (!ensure(IsValid(ItemMeshComponent))) return;

		{
			// Reset state to default
			DefaultCube->SetVisibility(false);
			if (IsValid(ItemActor))
			{
				ItemActor->Destroy();
				ItemActor = nullptr;
			}
			ItemMeshComponent->ClearItemMesh();
		}

		const UFaerieItem* FaerieItem = ItemProxy ? ItemProxy->GetItemObject() : nullptr;
		if (!IsValid(FaerieItem)) return;

		const UFaerieVisualActorClassToken* ActorClassToken = FaerieItem->GetToken<UFaerieVisualActorClassToken>();
		const UFaerieMeshTokenBase* MeshToken = FaerieItem->GetToken<UFaerieMeshTokenBase>();
		const UFaerieCapacityToken* CapToken = FaerieItem->GetToken<UFaerieCapacityToken>();

		// Draw Capacity Bounds
		if (CapToken)
		{
			BoundsBox->SetVisibility(ShowBounds);
			BoundsBox->SetBoxExtent(FVector(CapToken->GetCapacity().Bounds) / 2.0);
		}

		// Draw Mesh
		{
			// Path 1: Spawn Actor
			if (IsValid(ActorClassToken))
			{
				if (auto&& ActorClass = ActorClassToken->LoadProxyActorClassSynchronous())
				{
					ItemActor = Scene->GetWorld()->SpawnActor<AFaerieProxyActorBase>(ActorClass, FActorSpawnParameters());
					if (IsValid(ItemActor))
					{
						ItemActor->GetOnDisplayFinished().AddRaw(this, &FItemPreviewSceneData::OnDisplayFinished);

						FEditorScriptExecutionGuard ScriptGuard;
						ItemActor->SetSourceProxy(ItemProxy);
						return;
					}
				}
			}

			// Path 2: Spawn Component
			if (IsValid(MeshToken))
			{
				ItemMeshComponent->SetItemMeshFromToken(MeshToken);
				return;
			}

			// If the code above doesn't evaluate to a mesh, show the debug cube.
			DefaultCube->SetVisibility(true);
		}
	}

	FBoxSphereBounds FItemPreviewSceneData::GetBounds() const
	{
		return BoundsBox->GetLocalBounds();
	}

	void FItemPreviewSceneData::OnDisplayFinished(const bool Success)
	{
		if (!Success)
		{
			return;
		}

		// Run manual centering logic to keep the actor centered in view.
		const FBox Bounds = ItemActor->GetComponentsBoundingBox(true);
		ItemActor->AddActorLocalOffset(-Bounds.GetCenter());
	}

	FItemDataProxyPreviewScene::FItemDataProxyPreviewScene(ConstructionValues CVS)
	  : FAdvancedPreviewScene(CVS),
		SceneData(this)
	{
		// Hide default floor
		SetFloorVisibility(false, false);

		SceneData.InitializeScene();
	}

	void FItemDataProxyPreviewScene::Tick(const float InDeltaTime)
	{
		FAdvancedPreviewScene::Tick(InDeltaTime);

		if (GEditor->bIsSimulatingInEditor ||
			GEditor->PlayWorld != nullptr)
		{
			return;
		}

		GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}

	void FItemDataProxyPreviewScene::SetSettings(UFaerieItemAssetEditorCustomSettings* Settings)
	{
		EditorSettings = Settings;
		SyncSettings();
	}

	void FItemDataProxyPreviewScene::SyncSettings()
	{
		SceneData.SetShowBounds(EditorSettings->ShowCapacityBounds);
		SceneData.SetMeshPurposeTag(EditorSettings->MeshPreviewTag);
	}

	FBoxSphereBounds FItemDataProxyPreviewScene::GetBounds() const
	{
		return SceneData.GetBounds();
	}

	void FItemDataProxyPreviewScene::SetItemProxy(const IFaerieItemDataProxy* Proxy)
	{
		SceneData.SetProxy(Proxy);
	}

	void FItemDataProxyPreviewScene::RefreshMesh()
	{
		SceneData.RefreshItemData();
	}
}
