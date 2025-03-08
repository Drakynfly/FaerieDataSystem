// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetEditor/FaerieItemAssetViewport.h"
#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetViewportClient.h"
#include "CompGeom/DiTOrientedBox.h"

void SFaerieItemAssetViewport::Construct(const FArguments& InArgs, const FFaerieItemAssetViewportRequiredArgs& InRequiredArgs)
{
	EditorPtr = InRequiredArgs.AssetEditorToolkit;
	PreviewScene = InRequiredArgs.PreviewScene;
	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SFaerieItemAssetViewport::~SFaerieItemAssetViewport()
{
	if (TypedViewportClient.IsValid())
	{
		TypedViewportClient->Viewport = nullptr;
	}
}

TSharedRef<class SEditorViewport> SFaerieItemAssetViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SFaerieItemAssetViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShared<FExtender>());
	return Result;
}

void SFaerieItemAssetViewport::OnFloatingButtonClicked()
{
	// Nothing
}

void SFaerieItemAssetViewport::OnFocusViewportToSelection()
{
	if (TypedViewportClient.IsValid())
	{
		TypedViewportClient->FocusViewport(false);
	}
}

TSharedRef<FEditorViewportClient> SFaerieItemAssetViewport::MakeEditorViewportClient()
{
	TypedViewportClient = MakeShared<FFaerieItemAssetViewportClient>(SharedThis(this), PreviewScene.ToSharedRef());
	return TypedViewportClient.ToSharedRef();
}

