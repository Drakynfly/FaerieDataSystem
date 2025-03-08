// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AdvancedPreviewScene.h"

class FFaerieItemAssetEditor;

/**
 * 
 */
class FFaerieItemAssetPreviewScene : public FAdvancedPreviewScene
{
public:
	FFaerieItemAssetPreviewScene(ConstructionValues CVS, const TSharedRef<FFaerieItemAssetEditor>& EditorToolkit);
	virtual ~FFaerieItemAssetPreviewScene() override;

	//~ FAdvancedPreviewScene
	virtual void Tick(float InDeltaTime) override;
	//~ FAdvancedPreviewScene

	USceneComponent* GetSceneComponent() const { return PreviewComponent; };

	TSharedRef<FFaerieItemAssetEditor> GetEditor() const
	{
		return EditorPtr.Pin().ToSharedRef();
	}

	void RefreshMesh();

private:
	TObjectPtr<UStaticMeshComponent> PreviewComponent = nullptr;

	TWeakPtr<FFaerieItemAssetEditor> EditorPtr;
};
