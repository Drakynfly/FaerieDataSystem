// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetEditor.h"

#include "FaerieItemAsset.h"
#include "FaerieItemMeshLoader.h"
#include "AssetEditor/FaerieWidgetPreview.h"
#include "Components/BoxComponent.h"
#include "Components/FaerieItemMeshComponent.h"

#include "GameFramework/WorldSettings.h"
#include "Tokens/FaerieCapacityToken.h"
#include "Tokens/FaerieMeshToken.h"
#include "Tokens/FaerieVisualActorClassToken.h"

FFaerieItemAssetPreviewScene::FFaerieItemAssetPreviewScene(ConstructionValues CVS, const TSharedRef<FFaerieItemAssetEditor>& EditorToolkit)
	: FAdvancedPreviewScene(CVS)
	, EditorPtr(EditorToolkit)
{
	// Hide default floor
	SetFloorVisibility(false, false);

	auto World = GetWorld();

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
		
		FFaerieItemAssetPreviewScene::AddComponent(DefaultCube, FTransform::Identity);
	}

	{
		BoundsBox = NewObject<UBoxComponent>(GetTransientPackage());
		BoundsBox->SetLineThickness(0.2f);
		BoundsBox->SetVisibility(false);

		FFaerieItemAssetPreviewScene::AddComponent(BoundsBox, FTransform::Identity);
	}

	{
		Actor = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	{
		ItemMeshComponent = NewObject<UFaerieItemMeshComponent>(Actor);
		ItemMeshComponent->CenterMeshByBounds = true;
		//ItemMeshComponent->bSelectable = true;

		FFaerieItemAssetPreviewScene::AddComponent(ItemMeshComponent, FTransform::Identity);
	}

	MeshLoader = NewObject<UFaerieItemMeshLoader>(Actor, MakeUniqueObjectName(Actor, UFaerieItemMeshLoader::StaticClass()));
}

FFaerieItemAssetPreviewScene::~FFaerieItemAssetPreviewScene()
{
}

void FFaerieItemAssetPreviewScene::AddReferencedObjects(FReferenceCollector& Collector)
{
	FAdvancedPreviewScene::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(MeshLoader);
}

void FFaerieItemAssetPreviewScene::Tick(const float InDeltaTime)
{
	FAdvancedPreviewScene::Tick(InDeltaTime);

	if (const TSharedPtr<FFaerieItemAssetEditor> Toolkit = EditorPtr.Pin())
	{
		if (GEditor->bIsSimulatingInEditor ||
			GEditor->PlayWorld != nullptr)
		{
			return;
		}

		GetWorld()->Tick(LEVELTICK_All, InDeltaTime);
	}
}

FBoxSphereBounds FFaerieItemAssetPreviewScene::GetBounds() const
{
	return Actor->GetComponentsBoundingBox(true);
}

void FFaerieItemAssetPreviewScene::RefreshMesh()
{
	if (!ensure(IsValid(ItemMeshComponent))) return;
	if (!ensure(IsValid(MeshLoader))) return;

	ItemMeshComponent->ClearItemMesh();

	const UFaerieItemAsset* ItemAsset = EditorPtr.Pin()->GetItemAsset();
	if (!IsValid(ItemAsset)) return;

	const UFaerieVisualActorClassToken* ActorClassToken = nullptr;
	const UFaerieMeshTokenBase* MeshToken = nullptr;
	const UFaerieCapacityToken* CapToken = nullptr;

	for (auto&& Element : ItemAsset->GetEditorTokensView())
	{
		if (auto AsActorClassToken = Cast<UFaerieVisualActorClassToken>(Element))
		{
			ActorClassToken = AsActorClassToken;
			continue;
		}

		if (auto AsMeshToken = Cast<UFaerieMeshTokenBase>(Element))
		{
			MeshToken = AsMeshToken;
			continue;
		}

		if (auto AsCapToken = Cast<UFaerieCapacityToken>(Element))
		{
			CapToken = AsCapToken;
			continue;
		}
	}

	// Draw Capacity Bounds
	if (CapToken)
	{
		BoundsBox->SetVisibility(true); // @todo expose to settings panel
		BoundsBox->SetBoxExtent(FVector(CapToken->GetCapacity().Bounds) / 2.0);
	}

	// Draw Mesh
	{
		DefaultCube->SetVisibility(false);
		if (IsValid(ItemActor))
		{
			ItemActor->Destroy();
		}
		ItemMeshComponent->ClearItemMesh();

		if (IsValid(ActorClassToken))
		{
			if (auto ActorClass = ActorClassToken->LoadActorClassSynchronous())
			{
				ItemActor = GetWorld()->SpawnActor<AItemRepresentationActor>(ActorClass, FActorSpawnParameters());
				ItemActor->SetSourceProxy(EditorPtr.Pin()->GetPreview()); // @todo extremely stupid
				return;
			}
		}

		if (IsValid(MeshToken))
		{
			// @todo expose MeshPurpose to AssetEditor
			const FGameplayTag MeshPurpose = Faerie::ItemMesh::Tags::MeshPurpose_Default;

			if (FFaerieItemMesh ItemMesh;
				MeshLoader->LoadMeshFromTokenSynchronous(MeshToken, MeshPurpose, ItemMesh))
			{
				ItemMeshComponent->SetItemMesh(ItemMesh);
				return;
			}
		}

		// If the code above doesn't evaluate to a mesh, show the debug cube.
		DefaultCube->SetVisibility(true);
	}
}
