// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

namespace Faerie::Ed
{
	class FItemAssetEditorToolkit;
	class FItemDataProxyPreviewScene;
	class FItemAssetViewportClient;

	struct FFaerieItemAssetViewportRequiredArgs
	{
		FFaerieItemAssetViewportRequiredArgs(const TSharedRef<FItemDataProxyPreviewScene>& InPreviewScene, const TSharedRef<FItemAssetEditorToolkit>& InAssetEditorToolkit)
		: PreviewScene(InPreviewScene)
		, AssetEditorToolkit(InAssetEditorToolkit)
		{
		}

		TSharedRef<FItemDataProxyPreviewScene> PreviewScene;
		TSharedRef<FItemAssetEditorToolkit> AssetEditorToolkit;
	};

	/**
	 *
	 */
	class SItemAssetEditorViewport final : public SEditorViewport, /*public FGCObject,*/ public ICommonEditorViewportToolbarInfoProvider
	{
	public:
		SLATE_BEGIN_ARGS(SItemAssetEditorViewport) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const FFaerieItemAssetViewportRequiredArgs& InRequiredArgs);
		virtual ~SItemAssetEditorViewport() override;

		//~ SEditorViewport
		virtual void OnFocusViewportToSelection() override;
		virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
		//~ SEditorViewport

		//virtual void AddReferencedObjects(FReferenceCollector& Collector) override {}

		//~ ICommonEditorViewportToolbarInfoProvider
		virtual TSharedRef<SEditorViewport> GetViewportWidget() override;
		virtual TSharedPtr<FExtender> GetExtenders() const override;
		virtual void OnFloatingButtonClicked() override;
		//~ ICommonEditorViewportToolbarInfoProvider

		TSharedPtr<FItemAssetViewportClient> GetViewportClient() { return TypedViewportClient; };

	private:
		/** The scene for this viewport. */
		TSharedPtr<FItemDataProxyPreviewScene> PreviewScene;

		//Shared ptr to the client
		TSharedPtr<FItemAssetViewportClient> TypedViewportClient;

		//Toolkit Pointer
		TSharedPtr<FItemAssetEditorToolkit> EditorPtr;
	};
}