// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "EditorViewportClient.h"

namespace Faerie::Editor
{
	class FItemDataProxyPreviewScene;
	class SItemAssetEditorViewport;

	/**
	* FEditorViewportClient is generally responsible for handling the viewport, camera movement, and
	* any of the options you'd find under the "Lit" menu on the standard Unreal Engine main viewport.
	*/
	class FItemAssetViewportClient final : public FEditorViewportClient
	{
	public:
		/* Constructor and destructor */
		FItemAssetViewportClient(const TSharedRef<SItemAssetEditorViewport>& InThumbnailViewport, const TSharedRef<FItemDataProxyPreviewScene>& InPreviewScene);

		/* Shameless stolen from SMaterialEditorViewport because for some reason a basic Focus on Selection is not implemented in the ViewportClient base class. */
		void FocusViewport(bool bInstant /*= false*/);
		void FocusViewportOnBounds(const FBoxSphereBounds& Bounds, bool bInstant /*= false*/);

	private:
		/** Pointer back to the Editor Viewport */
		TWeakPtr<class SItemAssetEditorViewport> ViewportPtr;

		/* Stored pointer to the preview scene in which the static mesh is shown */
		TSharedPtr<FItemDataProxyPreviewScene> AdvancedPreviewScene;
	};
}
