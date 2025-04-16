// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AdvancedPreviewScene.h"

class UFaerieItemMeshLoader;
class UFaerieItemMeshComponent;
class FFaerieItemAssetEditor;

/**
 * 
 */
class FFaerieItemAssetPreviewScene : public FAdvancedPreviewScene
{
public:
	FFaerieItemAssetPreviewScene(ConstructionValues CVS, const TSharedRef<FFaerieItemAssetEditor>& EditorToolkit);
	virtual ~FFaerieItemAssetPreviewScene() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	//~ FAdvancedPreviewScene
	virtual void Tick(float InDeltaTime) override;
	//~ FAdvancedPreviewScene

	FBoxSphereBounds GetBounds() const;

	TSharedRef<FFaerieItemAssetEditor> GetEditor() const
	{
		return EditorPtr.Pin().ToSharedRef();
	}

	void RefreshMesh();

private:
	TObjectPtr<AActor> Actor;
	TObjectPtr<UStaticMeshComponent> DefaultCube = nullptr;
	TObjectPtr<UFaerieItemMeshComponent> ItemMeshComponent;
	TObjectPtr<UFaerieItemMeshLoader> MeshLoader;

	TWeakPtr<FFaerieItemAssetEditor> EditorPtr;
};
