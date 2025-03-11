// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetEditor.h"

#include "FaerieItem.h"
#include "FaerieItemAsset.h"

#include "GameFramework/WorldSettings.h"
#include "Tokens/FaerieMeshToken.h"

FFaerieItemAssetPreviewScene::FFaerieItemAssetPreviewScene(ConstructionValues CVS, const TSharedRef<FFaerieItemAssetEditor>& EditorToolkit)
	: FAdvancedPreviewScene(CVS)
	, EditorPtr(EditorToolkit)

{
	// Disable killing actors outside of the world
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings(true);
	WorldSettings->bEnableWorldBoundsChecks = false;

	//Hide default floor
	SetFloorVisibility(false, false);

	UStaticMesh* PreviewMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/EngineMeshes/Cube.Cube"), nullptr, LOAD_None, nullptr);
	FTransform PreviewMeshTransform (FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1.0f, 1.0f, 1.0f ));
	
	{
		PreviewComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
		PreviewComponent->SetStaticMesh(PreviewMesh);
		PreviewComponent->bSelectable = true;
		
		AddComponent(PreviewComponent, PreviewMeshTransform);
	}
}

FFaerieItemAssetPreviewScene::~FFaerieItemAssetPreviewScene()
{
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

void FFaerieItemAssetPreviewScene::RefreshMesh()
{
	UFaerieItemAsset* ItemAsset = EditorPtr.Pin()->GetItemAsset();
	if (!IsValid(ItemAsset)) return;

	FGameplayTagContainer PurposeHierarchy;
	PurposeHierarchy.AddTagFast(Faerie::ItemMesh::Tags::MeshPurpose_Default);

	const UFaerieMeshTokenBase* MeshToken = nullptr;
	for (auto&& Element : ItemAsset->GetEditorTokensView())
	{
		MeshToken = Cast<UFaerieMeshTokenBase>(Element);
		if (MeshToken)
		{
			break;
		}
	}

	if (IsValid(MeshToken))
	{
		FFaerieStaticMeshData MeshData;
		MeshToken->GetStaticItemMesh(PurposeHierarchy, MeshData);
		if (UStaticMesh* StaticMesh = MeshData.StaticMesh.LoadSynchronous();
			IsValid(StaticMesh))
		{
			PreviewComponent->SetStaticMesh(StaticMesh);
			for (auto It = MeshData.Materials.CreateConstIterator(); It; ++It)
			{
				PreviewComponent->SetMaterial(It.GetIndex(), It->Material.LoadSynchronous());
			}
		}
	}
}
