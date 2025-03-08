// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FaerieItemDataProxy.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

enum class EWidgetPreviewWidgetChangeType : uint8;
class UFaerieItemDataStackLiteral;
class FFaerieItemAssetEditor;

namespace Faerie::UMGWidgetPreview
{
	class SWidgetPreview
		: public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWidgetPreview) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& Args, const TSharedRef<FFaerieItemAssetEditor>& InToolkit);

		virtual ~SWidgetPreview() override;

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		//void OnStateChanged(FWidgetPreviewToolkitStateBase* InOldState, FWidgetPreviewToolkitStateBase* InNewState);
		void OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType);

		/** Convenience method to get world from the associated viewport. */
		UWorld* GetWorld() const;

		TSharedRef<SWidget> GetCreatedSlateWidget() const;

	private:
		TWeakPtr<FFaerieItemAssetEditor> WeakToolkit;

		TSharedPtr<SRetainerWidget> RetainerWidget;
		TSharedPtr<SBorder> ContainerWidget;
		TWeakPtr<SWidget> CreatedSlateWidget;

		bool bClearWidgetOnNextPaint = false;
		bool bIsRetainedRender = false;

		FDelegateHandle OnStateChangedHandle;
		FDelegateHandle OnWidgetChangedHandle;
	};
}


class UFaerieWidgetPreview;
class FFaerieItemAssetPreviewScene;
class SFaerieItemAssetViewport;
class UFaerieItemAsset;

/**
 * 
 */
class FFaerieItemAssetEditor : public FAssetEditorToolkit, public FGCObject, public FNotifyHook
{
public:
	FFaerieItemAssetEditor() = default;
	virtual ~FFaerieItemAssetEditor() override = default;

protected:
	//~ Begin IToolkit interface
	virtual FName GetToolkitFName() const override { return "FaerieItemAssetEditor"; }
	virtual FText GetBaseToolkitName() const override { return INVTEXT("Faerie Item Asset Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return "FaerieItemAsset"; }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return {}; }
	//~ End IToolkit interface

	//~ FAssetEditorToolkit
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void OnClose() override;
	//~ FAssetEditorToolkit

	//~ FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	virtual FString GetReferencerName() const override;
	//~ FGCObject interface

	//~ FNotifyHook
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) override;
	//~ FNotifyHook


	/**		 SETUP		 */
public:
	void InitAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFaerieItemAsset* InItemAsset);

protected:
	void BindCommands();
	void ExtendToolbars();
	TSharedRef<FFaerieItemAssetPreviewScene> CreatePreviewScene();
	UFaerieWidgetPreview* CreateWidgetPreview();

	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args) const;
	TSharedRef<SDockTab> SpawnTab_WidgetPreview(const FSpawnTabArgs& Args) const;


	/**		 GETTERS		 */
public:
	UFaerieItemAsset* GetItemAsset() const { return ItemAsset; }
	UFaerieWidgetPreview* GetPreview() const { return WidgetPreview; }

	UWorld* GetPreviewWorld() const;


	/**		 ACTIONS		 */
protected:
	void FocusViewport() const;

private:
	TObjectPtr<UFaerieItemAsset> ItemAsset = nullptr;

	TObjectPtr<UFaerieWidgetPreview> WidgetPreview = nullptr;
	TSharedPtr<FFaerieItemAssetPreviewScene> PreviewScene;

	TSharedPtr<SFaerieItemAssetViewport> MeshViewportWidget;
	TSharedPtr<SWidget> WidgetPreviewWidget;
};
