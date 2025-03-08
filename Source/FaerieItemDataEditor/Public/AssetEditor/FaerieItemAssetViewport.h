// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SCommonEditorViewportToolbarBase.h"
#include "SEditorViewport.h"

class FFaerieItemAssetEditor;
class FFaerieItemAssetPreviewScene;

struct FFaerieItemAssetViewportRequiredArgs
{
	FFaerieItemAssetViewportRequiredArgs(const TSharedRef<FFaerieItemAssetPreviewScene>& InPreviewScene, TSharedRef<FFaerieItemAssetEditor> InAssetEditorToolkit)
	: PreviewScene(InPreviewScene)
	, AssetEditorToolkit(InAssetEditorToolkit)
	{
	}

	TSharedRef<FFaerieItemAssetPreviewScene> PreviewScene;
	TSharedRef<FFaerieItemAssetEditor> AssetEditorToolkit;
};

class FFaerieItemAssetViewportClient;

/**
 * 
 */
class SFaerieItemAssetViewport : public SEditorViewport, /*public FGCObject,*/ public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(SFaerieItemAssetViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const FFaerieItemAssetViewportRequiredArgs& InRequiredArgs);
	virtual ~SFaerieItemAssetViewport() override;

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

	TSharedPtr<FFaerieItemAssetViewportClient> GetViewportClient() { return TypedViewportClient; };

private:
	/** The scene for this viewport. */
	TSharedPtr<FFaerieItemAssetPreviewScene> PreviewScene;

	//Shared ptr to the client
	TSharedPtr<FFaerieItemAssetViewportClient> TypedViewportClient;

	//Toolkit Pointer
	TSharedPtr<FFaerieItemAssetEditor> EditorPtr;
};
