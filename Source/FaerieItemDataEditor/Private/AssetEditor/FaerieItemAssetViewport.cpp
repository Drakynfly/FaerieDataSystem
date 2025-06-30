// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetViewport.h"
#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetViewportClient.h"

namespace Faerie::Ed
{
	void SItemAssetEditorViewport::Construct(const FArguments& InArgs, const FFaerieItemAssetViewportRequiredArgs& InRequiredArgs)
	{
		EditorPtr = InRequiredArgs.AssetEditorToolkit;
		PreviewScene = InRequiredArgs.PreviewScene;
		SEditorViewport::Construct(SEditorViewport::FArguments());
	}

	SItemAssetEditorViewport::~SItemAssetEditorViewport()
	{
		if (TypedViewportClient.IsValid())
		{
			TypedViewportClient->Viewport = nullptr;
		}
	}

	TSharedRef<class SEditorViewport> SItemAssetEditorViewport::GetViewportWidget()
	{
		return SharedThis(this);
	}

	TSharedPtr<FExtender> SItemAssetEditorViewport::GetExtenders() const
	{
		TSharedPtr<FExtender> Result(MakeShared<FExtender>());
		return Result;
	}

	void SItemAssetEditorViewport::OnFloatingButtonClicked()
	{
		// Nothing
	}

	void SItemAssetEditorViewport::OnFocusViewportToSelection()
	{
		if (TypedViewportClient.IsValid())
		{
			TypedViewportClient->FocusViewport(false);
		}
	}

	TSharedRef<FEditorViewportClient> SItemAssetEditorViewport::MakeEditorViewportClient()
	{
		TypedViewportClient = MakeShared<FItemAssetViewportClient>(SharedThis(this), PreviewScene.ToSharedRef());
		return TypedViewportClient.ToSharedRef();
	}
}
