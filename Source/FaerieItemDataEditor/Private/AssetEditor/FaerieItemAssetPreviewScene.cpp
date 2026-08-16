// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "Editor.h"
#include "FaerieItem.h"
#include "FaerieItemDataView.h"

#include "Actors/FaerieProxyActorBase.h"

#include "Components/BoxComponent.h"
#include "Components/FaerieItemMeshComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Fragments/FaerieActorFragment.h"
#include "Capacity/CapacityStructs.h"

#include "GameFramework/WorldSettings.h"

namespace Faerie::Editor
{
	FItemPreviewSceneData::FItemPreviewSceneData(FPreviewScene* Scene)
	  : Scene(Scene)
	{
		MeshPurposeTag = Mesh::Tags::MeshPurpose_Default;
	}

	FItemPreviewSceneData::~FItemPreviewSceneData()
	{
		if (AFaerieProxyActorBase* Actor = ItemActor.Get())
		{
			Actor->Destroy();
			ItemActor.Reset();
		}

		if (UFaerieItemMeshComponent* MeshComponent = ItemMeshComponent.Get())
		{
			if (!MeshComponent->HasAnyFlags(RF_BeginDestroyed))
			{
				MeshComponent->ClearItemMesh();
				MeshComponent->DestroyComponent();
			}

			ItemMeshComponent.Reset();
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

			Scene->AddComponent(DefaultCube.Get(), FTransform::Identity);
		}

		{
			BoundsBox = NewObject<UBoxComponent>(GetTransientPackage());
			BoundsBox->SetLineThickness(0.2f);
			BoundsBox->SetVisibility(false);

			Scene->AddComponent(BoundsBox.Get(), FTransform::Identity);
		}

		{
			ItemMeshComponent = NewObject<UFaerieItemMeshComponent>(GetTransientPackage());
			ItemMeshComponent->CenterMeshByBounds = CenterMeshByBounds;
			ItemMeshComponent->CacheSkeletalBoundsInPose = true;
			ItemMeshComponent->PreferredTag = MeshPurposeTag;
			ItemMeshComponent->OnMeshRebuiltNative.AddWeakLambda(World, [this](const TNotNull<UFaerieItemMeshComponent*> Component)
			{
				// Show the default cube when we do not have a valid mesh generated.
				if (Component->GetMeshType() == EItemMeshType::None)
				{
					DefaultCube->SetVisibility(true);
				}
			});
			//ItemMeshComponent->bSelectable = true;

			Scene->AddComponent(ItemMeshComponent.Get(), FTransform::Identity);
		}
	}

	void FItemPreviewSceneData::SetProxy(const FFaerieItemProxy& Proxy)
	{
		ItemProxy = Proxy;
		RefreshItemData();
	}

	void FItemPreviewSceneData::SetShowBounds(const bool InShowBounds)
	{
		ShowBounds = InShowBounds;
		if (BoundsBox.IsValid())
		{
			BoundsBox->SetVisibility(ShowBounds);
		}
	}

	void FItemPreviewSceneData::SetMeshPurposeTag(const FGameplayTag Tag)
	{
		MeshPurposeTag = Tag;
		if (ItemMeshComponent.IsValid())
		{
			ItemMeshComponent->SetPreferredTag(MeshPurposeTag);
		}
	}

	void FItemPreviewSceneData::RefreshItemData()
	{
		if (!ensure(ItemMeshComponent.IsValid())) return;

		{
			// Reset state to default
			DefaultCube->SetVisibility(false);
			if (AFaerieProxyActorBase* Actor = ItemActor.Get())
			{
				Actor->Destroy();
				ItemActor.Reset();
			}
			ItemMeshComponent->ClearItemMesh();
		}

		if (!ItemProxy.IsValid()) return;

		const FFaerieItemInstance Instance = ItemProxy.GetItemInstance().GetValue();
		const UFaerieItem* FaerieItem = Instance.GetItemPtr();
		if (!IsValid(FaerieItem)) return;

		auto ProxyClassFragment = Faerie::ItemData::GetDefaultFragment<FFaerieProxyActorFragment>(FaerieItem);
		auto Capacity = Faerie::ItemData::GetDefaultFragment<FFaerieItemCapacity>(FaerieItem);

		// Draw Capacity Bounds
		if (Capacity.IsValid())
		{
			BoundsBox->SetVisibility(ShowBounds);
			BoundsBox->SetBoxExtent(FVector(Capacity->Bounds) / 2.0);
		}

		// Draw Mesh
		{
			// Path 1: Spawn Actor
			if (ProxyClassFragment.IsValid())
			{
				if (auto&& ActorClass = ProxyClassFragment->LoadProxyActorClassSynchronous())
				{
					ItemActor = Scene->GetWorld()->SpawnActor<AFaerieProxyActorBase>(ActorClass, FActorSpawnParameters());
					if (AFaerieProxyActorBase* Actor = ItemActor.Get())
					{
						Actor->GetOnDisplayFinished().AddRaw(this, &FItemPreviewSceneData::OnDisplayFinished);

						FEditorScriptExecutionGuard ScriptGuard;
						Actor->SetSourceProxy(ItemProxy);
						return;
					}
				}
			}

			// Path 2: Spawn Component
			ItemMeshComponent->SetItemMeshFromProxy(ItemProxy);
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

	void FItemDataProxyPreviewScene::SetItemProxy(const FFaerieItemProxy& Proxy)
	{
		SceneData.SetProxy(Proxy);
	}

	void FItemDataProxyPreviewScene::RefreshMesh()
	{
		SceneData.RefreshItemData();
	}
}
