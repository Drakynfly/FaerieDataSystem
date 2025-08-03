// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemAssetEditorCustomSettings.h"
#include "Misc/NotifyHook.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"

enum class EWidgetPreviewWidgetChangeType : uint8;
class UFaerieItemAsset;
class UFaerieItemAssetEditorCustomSettings;
class UFaerieWidgetPreview;

namespace Faerie::UMGWidgetPreview
{
	struct FWidgetPreviewToolkitStateBase;
}

namespace Faerie::Ed
{
	class FItemDataProxyPreviewScene;
	class SItemAssetEditorViewport;

	using FOnStateChanged = TMulticastDelegate<void(UMGWidgetPreview::FWidgetPreviewToolkitStateBase* InOldState, UMGWidgetPreview::FWidgetPreviewToolkitStateBase* InNewState)>;

	/**
	 *
	 */
	class FItemAssetEditorToolkit : public FAssetEditorToolkit, public FGCObject, public FNotifyHook
	{
	public:
		FItemAssetEditorToolkit() = default;
		virtual ~FItemAssetEditorToolkit() override;

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
		virtual void PostInitAssetEditor() override;
		virtual void SaveAsset_Execute() override;
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
		TSharedRef<FItemDataProxyPreviewScene> CreatePreviewScene();
		UFaerieWidgetPreview* CreateWidgetPreview();

		TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args) const;
		TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args) const;
		TSharedRef<SDockTab> SpawnTab_WidgetPreview(const FSpawnTabArgs& Args) const;
		TSharedRef<SDockTab> SpawnTab_SceneSettings(const FSpawnTabArgs& Args) const;

		bool ShouldUpdate() const;

		void OnBlueprintPrecompile(UBlueprint* InBlueprint);

		void OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType);

		void OnFocusChanging(
			const FFocusEvent& InFocusEvent,
			const FWeakWidgetPath& InOldWidgetPath, const TSharedPtr<SWidget>& InOldWidget,
			const FWidgetPath& InNewWidgetPath, const TSharedPtr<SWidget>& InNewWidget);

		void OnSceneSettingsChanged(const FEditorCustomSettingsEventData& Data);

		/** Resolve and set the current state based on various conditions. */
		void ResolveState();

		/** Resets to the default state. */
		void ResetPreview();


		/**		 GETTERS		 */
	public:
		FOnStateChanged::RegistrationType& OnStateChanged() { return OnStateChangedDelegate; }
		UMGWidgetPreview::FWidgetPreviewToolkitStateBase* GetState() const { return CurrentState; }

		UFaerieItemAsset* GetItemAsset() const { return ItemAsset; }
		UFaerieWidgetPreview* GetPreview() const { return WidgetPreview; }

		UWorld* GetPreviewWorld() const;


		/**		 ACTIONS		 */
	protected:
		void FocusViewport() const;

		/** If the given state is different to the current state, this will handle transitions and events. */
		void SetState(UMGWidgetPreview::FWidgetPreviewToolkitStateBase* InNewState);

	private:
		TObjectPtr<UFaerieItemAsset> ItemAsset = nullptr;
		TObjectPtr<UFaerieWidgetPreview> WidgetPreview = nullptr;
		TObjectPtr<UFaerieItemAssetEditorCustomSettings> CustomSceneSettings = nullptr;

		TSharedPtr<FItemDataProxyPreviewScene> PreviewScene;

		TSharedPtr<SItemAssetEditorViewport> MeshViewportWidget;
		TSharedPtr<SWidget> WidgetPreviewWidget;
		TSharedPtr<SWidget> PreviewSettingsWidget;

		FOnStateChanged OnStateChangedDelegate;

		UMGWidgetPreview::FWidgetPreviewToolkitStateBase* CurrentState = nullptr;

		bool bIsFocused = false;

		FDelegateHandle OnBlueprintPrecompileHandle;
		FDelegateHandle OnWidgetChangedHandle;
		FDelegateHandle OnFocusChangingHandle;
	};
}
