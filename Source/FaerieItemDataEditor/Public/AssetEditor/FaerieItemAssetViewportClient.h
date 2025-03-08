// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EditorViewportClient.h"

class FFaerieItemAssetPreviewScene;
class SFaerieItemAssetViewport;

/**
 * FEditorViewportClient is generally responsible for handling the viewport, camera movement, and
 * any of the options you'd find under the "Lit" menu on the standard Unreal Engine main viewport.
 */
class FFaerieItemAssetViewportClient : public FEditorViewportClient
{
public:
	/* Constructor and destructor */
	FFaerieItemAssetViewportClient(const TSharedRef<SFaerieItemAssetViewport>& InThumbnailViewport, const TSharedRef<FFaerieItemAssetPreviewScene>& InPreviewScene);

	/* Shameless stolen from SMaterialEditorViewport because for some reason a basic Focus on Selection is not implemented in the ViewportClient base class. */
	void FocusViewport(bool bInstant /*= false*/);
	void FocusViewportOnBounds(const FBoxSphereBounds& Bounds, bool bInstant /*= false*/);

private:
	/** Pointer back to the Editor Viewport */
	TWeakPtr<class SFaerieItemAssetViewport> ViewportPtr;

	/* Stored pointer to the preview scene in which the static mesh is shown */
	TSharedPtr<FFaerieItemAssetPreviewScene> AdvancedPreviewScene;
};
