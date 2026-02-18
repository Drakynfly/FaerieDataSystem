// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "AssetEditor/FaerieItemAssetEditor.h"
#include "AdvancedPreviewSceneModule.h"
#include "FaerieAssetEditorCommands.h"
#include "FaerieCardSettings.h"
#include "FaerieItemAsset.h"
#include "FaerieItemCardTags.h"
#include "FaerieItemDataEditorModule.h"
#include "PropertyEditorModule.h"
#include "SWidgetPreview.h"
#include "SWidgetPreviewStatus.h"
#include "AssetEditor/FaerieItemAssetEditorCustomSettings.h"
#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetViewport.h"
#include "AssetEditor/FaerieWidgetPreview.h"
#include "Blueprint/UserWidget.h"
#include "CardTokens/FaerieItemCardToken.h"
#include "Engine/Blueprint.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FaerieItemAssetEditor"

static constexpr FLazyName Faerie_DetailsTab("FaerieItemAssetDetailsTab");
static constexpr FLazyName Faerie_ViewportTab("FaerieItemAssetViewportTab");
static constexpr FLazyName Faerie_PreviewSceneSettingsTab("FaerieItemAssetPreviewSceneSettingsTab");
static constexpr FLazyName Faerie_PreviewTab("FaerieItemAssetPreviewTab");

static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitPausedState PausedState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitBackgroundState BackgroundState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitUnsupportedWidgetState UnsupportedWidgetState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitRunningState RunningState;

namespace Faerie::Editor
{
	FItemAssetEditorToolkit::~FItemAssetEditorToolkit()
	{
		if (GEditor)
		{
			GEditor->OnBlueprintPreCompile().Remove(OnBlueprintPrecompileHandle);
		}

		if (WidgetPreview)
		{
			WidgetPreview->ClearWidgetInstance();
			WidgetPreview->OnWidgetChanged().Remove(OnWidgetChangedHandle);
		}

		// Ensure remaining references to the update state stop ticking
		SetState(&PausedState);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().OnFocusChanging().Remove(OnFocusChangingHandle);
		}
	}

	void FItemAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
	{
		FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

		WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Faerie Item Asset"));

		// Register Details
		InTabManager->RegisterTabSpawner(Faerie_DetailsTab,
			FOnSpawnTab::CreateSP(this, &FItemAssetEditorToolkit::SpawnTab_Details))
				.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

		// Register Viewport
		InTabManager->RegisterTabSpawner(Faerie_ViewportTab,
			FOnSpawnTab::CreateSP(this, &FItemAssetEditorToolkit::SpawnTab_Viewport))
				.SetDisplayName(LOCTEXT("MeshViewport", "Mesh Viewport"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

		// Register Preview
		InTabManager->RegisterTabSpawner(Faerie_PreviewTab,
			FOnSpawnTab::CreateSP(this, &FItemAssetEditorToolkit::SpawnTab_WidgetPreview))
				.SetDisplayName(LOCTEXT("WidgetPreviewViewport", "Widget Preview"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x")); // @todo widget icon!

		InTabManager->RegisterTabSpawner(Faerie_PreviewSceneSettingsTab,
			FOnSpawnTab::CreateSP(this, &FItemAssetEditorToolkit::SpawnTab_SceneSettings))
				.SetDisplayName(LOCTEXT("ScenePreviewTab", "Scene Settings"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"))
				.SetReadOnlyBehavior(ETabReadOnlyBehavior::Disabled);
	}

	void FItemAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
	{
		FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
		InTabManager->UnregisterTabSpawner(Faerie_DetailsTab);
		InTabManager->UnregisterTabSpawner(Faerie_ViewportTab);
		InTabManager->UnregisterTabSpawner(Faerie_PreviewTab);
		InTabManager->UnregisterTabSpawner(Faerie_PreviewSceneSettingsTab);
	}

	void FItemAssetEditorToolkit::PostInitAssetEditor()
	{
		FAssetEditorToolkit::PostInitAssetEditor();

		if (GEditor)
		{
			OnBlueprintPrecompileHandle = GEditor->OnBlueprintPreCompile().AddRaw(this, &FItemAssetEditorToolkit::OnBlueprintPrecompile);
		}

		OnWidgetChangedHandle = WidgetPreview->OnWidgetChanged().AddSP(this, &FItemAssetEditorToolkit::OnWidgetChanged);

		if (FSlateApplication::IsInitialized())
		{
			OnFocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddSP(this, &FItemAssetEditorToolkit::OnFocusChanging);
		}

		// Bind Commands
		{
			// @todo
			/*
			const FWidgetPreviewCommands& Commands = FWidgetPreviewCommands::Get();

			ToolkitCommands->MapAction(
				Commands.ResetPreview,
				FExecuteAction::CreateSP(this, &FFaerieItemAssetEditor::ResetPreview));
				*/
		}

		ResolveState();
	}

	void FItemAssetEditorToolkit::SaveAsset_Execute()
	{
		FAssetEditorToolkit::SaveAsset_Execute();

		if (PreviewScene.IsValid())
		{
			PreviewScene->RefreshMesh();
		}
	}

	void FItemAssetEditorToolkit::OnClose()
	{
		ItemAsset = nullptr;

		if (WidgetPreview)
		{
			WidgetPreview->ClearWidgetInstance();
			WidgetPreview = nullptr;
		}

		if (PreviewScene.IsValid())
		{
			PreviewScene.Reset();
		}

		if (MeshViewportWidget.IsValid())
		{
			MeshViewportWidget.Reset();
		}

		if (WidgetPreviewWidget.IsValid())
		{
			WidgetPreviewWidget.Reset();
		}
	}

	void FItemAssetEditorToolkit::AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(WidgetPreview);
		Collector.AddReferencedObject(CustomSceneSettings);
	}

	FString FItemAssetEditorToolkit::GetReferencerName() const
	{
		return TEXT("FFaerieItemAssetEditor");
	}

	void FItemAssetEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
												  FProperty* PropertyThatChanged)
	{
		FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

		if (PreviewScene.IsValid())
		{
			PreviewScene->RefreshMesh();
		}
	}

	void FItemAssetEditorToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
		class FEditPropertyChain* PropertyThatChanged)
	{
		FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

		if (PreviewScene.IsValid())
		{
			PreviewScene->RefreshMesh();
		}
	}

	void FItemAssetEditorToolkit::InitAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFaerieItemAsset* InItemAsset)
	{
		BindCommands();

		CustomSceneSettings = NewObject<UFaerieItemAssetEditorCustomSettings>();
		CustomSceneSettings->LoadConfig();
		CustomSceneSettings->GetOnSettingsChanged().BindSP(this, &FItemAssetEditorToolkit::OnSceneSettingsChanged);

		ItemAsset = InItemAsset;

		// Create preview scene viewport
		const FFaerieItemAssetViewportRequiredArgs Args(
			CreatePreviewScene(),
			SharedThis(this)
		);
		MeshViewportWidget = SNew(SItemAssetEditorViewport, Args);

		// @Note: because the widget preview uses the world from the preview scene, it has to be created second.

		// Create UMG preview widget
		CreateWidgetPreview();
		WidgetPreviewWidget =
			SNew(Faerie::UMGWidgetPreview::SWidgetPreview, SharedThis(this))
			.IsEnabled(this, &FItemAssetEditorToolkit::ShouldUpdate);

		// This is the only UObject we have easy access to that inherits from IFaerieItemProxy. Kinda hacky, because it means the widget preview has to exist for the scene preview to function.
		PreviewScene->SetItemProxy(WidgetPreview);

		TArray<FAdvancedPreviewSceneModule::FDetailDelegates> Delegates;
		//Delegates.Add({ OnPreviewSceneChangedDelegate });
		FAdvancedPreviewSceneModule& AdvancedPreviewSceneModule = FModuleManager::LoadModuleChecked<FAdvancedPreviewSceneModule>("AdvancedPreviewScene");
		PreviewSettingsWidget = AdvancedPreviewSceneModule.CreateAdvancedPreviewSceneSettingsWidget(
			PreviewScene.ToSharedRef(),
			CustomSceneSettings,
			TArray<FAdvancedPreviewSceneModule::FDetailCustomizationInfo>(),  TArray<FAdvancedPreviewSceneModule::FPropertyTypeCustomizationInfo>(),
			Delegates);

		const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("FaerieItemAssetEditor_Layout_v0.2")
			->AddArea
			(
				FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
					 ->Split
					 (
						 FTabManager::NewSplitter()
						 ->SetSizeCoefficient(1.f)
						 ->SetOrientation(Orient_Horizontal)
						 ->Split
						 (
							 FTabManager::NewStack()
							 ->SetSizeCoefficient(0.3f)
							 ->AddTab(Faerie_DetailsTab, ETabState::OpenedTab)
							 ->AddTab(Faerie_PreviewSceneSettingsTab, ETabState::OpenedTab)
							 ->SetForegroundTab(Faerie_DetailsTab.Resolve())
						 )
						 ->Split
						 (
							 FTabManager::NewStack()
							 ->SetSizeCoefficient(0.3f)
							 ->AddTab(Faerie_PreviewTab, ETabState::OpenedTab)
						 )->Split
						 (
							 FTabManager::NewStack()
							 ->SetSizeCoefficient(0.4f)
							 ->AddTab(Faerie_ViewportTab, ETabState::OpenedTab)
						 )
					 )
			);

		FAssetEditorToolkit::InitAssetEditor(EToolkitMode::Standalone, InitToolkitHost, "FaerieItemAssetEditor", Layout, true, true, ItemAsset);

		//Add buttons to the Asset Editor
		ExtendToolbars();

		//Focus the viewport on preview bounds
		FocusViewport();
	}

	void FItemAssetEditorToolkit::BindCommands()
	{
		const FAssetEditorCommands& Commands = FAssetEditorCommands::Get();

		ToolkitCommands->MapAction(Commands.FocusViewport, FExecuteAction::CreateSP(this, &FItemAssetEditorToolkit::FocusViewport));
	}

	void FItemAssetEditorToolkit::ExtendToolbars()
	{
		struct FLocal
		{
			static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection("ExtendToolbarItem");
				{
					ToolbarBuilder.AddToolBarButton(FAssetEditorCommands::Get().FocusViewport,
													NAME_None,
													LOCTEXT("FocusViewport", "Focus Viewport"),
													LOCTEXT("FocusViewportTooltip", "Focuses Viewport on selected Mesh"),
													FSlateIcon()
					);
				}
				ToolbarBuilder.EndSection();
			}
		};

		//Register Toolbar Extenders
		const TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();

		ToolbarExtender->AddToolBarExtension(
			"Asset",
			EExtensionHook::After,
			GetToolkitCommands(),
			FToolBarExtensionDelegate::CreateStatic(&FLocal::FillToolbar)
		);

		AddToolbarExtender(ToolbarExtender);

		FFaerieItemDataEditorModule* AssetEditorTemplateModule = &FModuleManager::LoadModuleChecked<FFaerieItemDataEditorModule>("FaerieItemDataEditor");
		AddToolbarExtender(AssetEditorTemplateModule->GetEditorToolbarExtensibilityManager()->GetAllExtenders());

		RegenerateMenusAndToolbars();
	}

	TSharedRef<FItemDataProxyPreviewScene> FItemAssetEditorToolkit::CreatePreviewScene()
	{
		// Create Preview Scene
		if (!PreviewScene.IsValid())
		{
			PreviewScene = MakeShared<FItemDataProxyPreviewScene>(
				FPreviewScene::ConstructionValues()
				.AllowAudioPlayback(true)
				.ShouldSimulatePhysics(true)
				.ForceUseMovementComponentInNonGameWorld(true));
		}

		PreviewScene->SetSettings(CustomSceneSettings);

		return PreviewScene.ToSharedRef();
	}

	UFaerieWidgetPreview* FItemAssetEditorToolkit::CreateWidgetPreview()
	{
		if (!IsValid(WidgetPreview))
		{
			FPreviewableWidgetVariant WidgetType;

			if (const UFaerieItemCardToken* CardToken = ItemAsset->GetItemInstance(EFaerieItemInstancingMutability::Immutable)->GetToken<UFaerieItemCardToken>())
			{
				WidgetType.ObjectPath = CardToken->GetCardClass(Card::Tags::CardType_Full).ToSoftObjectPath();
			}

			if (WidgetType.ObjectPath.IsNull())
			{
				// @todo temp
				if (auto Default = GetDefault<UFaerieCardSettings>()->FallbackClasses.Find(Card::Tags::CardType_Full))
				{
					WidgetType.ObjectPath = Default->ToSoftObjectPath();
				}
				else
				{
					return nullptr;
				}
			}

			WidgetPreview = NewObject<UFaerieWidgetPreview>();
			WidgetPreview->InitFaerieWidgetPreview(ItemAsset);
			WidgetPreview->SetWidgetType(WidgetType);
		}

		if (WidgetPreview)
		{
			WidgetPreview->GetOrCreateWidgetInstance(GetPreviewWorld(), true);
		}

		return WidgetPreview;
	}

	TSharedRef<SDockTab> FItemAssetEditorToolkit::SpawnTab_Details(const FSpawnTabArgs& Args) const
	{
		check(Args.GetTabId().TabType == Faerie_DetailsTab);

		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.NotifyHook = (FNotifyHook*)this;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		DetailsView->SetObjects(TArray<UObject*>{ ItemAsset });

		return SNew(SDockTab)
		[
			DetailsView
		];

		// @todo
		//const TSharedRef<FWidgetPreviewToolkitTemp>& Self = SharedThis(this);

		//return SNew(SDockTab)
		//[
		//	SNew(SWidgetPreviewDetails, Self)
		//];
	}

	TSharedRef<SDockTab> FItemAssetEditorToolkit::SpawnTab_Viewport(const FSpawnTabArgs& Args) const
	{
		check(Args.GetTabId().TabType == Faerie_ViewportTab);

		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("ViewportTab_Title", "Viewport"));

		if (MeshViewportWidget.IsValid())
		{
			SpawnedTab->SetContent(MeshViewportWidget.ToSharedRef());
			if (PreviewScene.IsValid())
			{
				PreviewScene->RefreshMesh();
			}
		}

		return SpawnedTab;
	}

	TSharedRef<SDockTab> FItemAssetEditorToolkit::SpawnTab_WidgetPreview(const FSpawnTabArgs& Args) const
	{
		TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("WidgetTab_Title", "Widget"));

		TSharedRef<FItemAssetEditorToolkit, ESPMode::ThreadSafe> Self = ConstCastSharedRef<FItemAssetEditorToolkit>( SharedThis(this) );

		if (WidgetPreviewWidget.IsValid())
		{
			SpawnedTab->SetContent(
				SNew(SOverlay)
				+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						WidgetPreviewWidget.ToSharedRef()
					]
				+ SOverlay::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(Faerie::UMGWidgetPreview::SWidgetPreviewStatus, Self)
					]);
		}

		return SpawnedTab;
	}

	TSharedRef<SDockTab> FItemAssetEditorToolkit::SpawnTab_SceneSettings(const FSpawnTabArgs& Args) const
	{
		check(Args.GetTabId().TabType == Faerie_PreviewSceneSettingsTab);

		return SNew(SDockTab)
			.Label( LOCTEXT("StaticMeshPreviewScene_TabTitle", "Scene Settings") )
			[
				PreviewSettingsWidget.IsValid() ? PreviewSettingsWidget.ToSharedRef() : SNullWidget::NullWidget
			];
	}

	bool FItemAssetEditorToolkit::ShouldUpdate() const
	{
		if (CurrentState)
		{
			return CurrentState->CanTick();
		}

		return bIsFocused;
	}

	void FItemAssetEditorToolkit::OnBlueprintPrecompile(UBlueprint* InBlueprint)
	{
		if (WidgetPreview)
		{
			if (const UUserWidget* WidgetCDO = WidgetPreview->GetWidgetCDO())
			{
				if (InBlueprint && InBlueprint->GeneratedClass
					&& WidgetCDO->IsA(InBlueprint->GeneratedClass))
				{
					WidgetPreview->ClearWidgetInstance();
				}
			}
		}
	}

	void FItemAssetEditorToolkit::OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType)
	{
		ResolveState();
	}

	void FItemAssetEditorToolkit::OnFocusChanging(
		const FFocusEvent& InFocusEvent,
		const FWeakWidgetPath& InOldWidgetPath, const TSharedPtr<SWidget>& InOldWidget,
		const FWidgetPath& InNewWidgetPath, const TSharedPtr<SWidget>& InNewWidget)
	{
		if (IsHosted())
		{
			const SWidget* ToolkitParentWidget = GetToolkitHost()->GetParentWidget().ToSharedPtr().Get();
			const bool bToolkitInNewWidgetPath = InNewWidgetPath.ContainsWidget(ToolkitParentWidget);
			if (bIsFocused && !bToolkitInNewWidgetPath)
			{
				// Focus lost
				bIsFocused = false;
				ResolveState();
			}
			else if (!bIsFocused && bToolkitInNewWidgetPath)
			{
				// Focus received
				bIsFocused = true;
				ResolveState();
			}
		}
	}

	void FItemAssetEditorToolkit::OnSceneSettingsChanged(const FEditorCustomSettingsEventData& Data)
	{
		PreviewScene->SyncSettings();
		CustomSceneSettings->SaveConfig();
	}

	void FItemAssetEditorToolkit::ResolveState()
	{
		UMGWidgetPreview::FWidgetPreviewToolkitStateBase* NewState = nullptr;

		if (!bIsFocused)
		{
			NewState = &BackgroundState;
		}
		else
		{
			TArray<const UUserWidget*> FailedWidgets;
			if (!GetPreview()->CanCallInitializedWithoutPlayerContext(true, FailedWidgets))
			{
				UnsupportedWidgetState.SetUnsupportedWidgets(FailedWidgets);
				NewState = &UnsupportedWidgetState;
			}

			// If we're here, the current state should be valid/running
			if (NewState == nullptr)
			{
				NewState = &RunningState;
			}
		}

		SetState(NewState);
	}

	void FItemAssetEditorToolkit::ResetPreview()
	{
		if (WidgetPreview)
		{
			// Don't need the returned instance, just need to have it rebuild
			WidgetPreview->GetOrCreateWidgetInstance(GetPreviewWorld(), true);
		}
	}

	UWorld* FItemAssetEditorToolkit::GetPreviewWorld() const
	{
		if (PreviewScene.IsValid())
		{
			return PreviewScene->GetWorld();
		}
		return nullptr;
	}

	void FItemAssetEditorToolkit::FocusViewport() const
	{
		if (MeshViewportWidget.IsValid())
		{
			MeshViewportWidget->OnFocusViewportToSelection();
		}
	}

	void FItemAssetEditorToolkit::SetState(UMGWidgetPreview::FWidgetPreviewToolkitStateBase* InNewState)
	{
		UMGWidgetPreview::FWidgetPreviewToolkitStateBase* OldState = CurrentState;
		UMGWidgetPreview::FWidgetPreviewToolkitStateBase* NewState = InNewState;

		if (OldState != NewState)
		{
			if (OldState)
			{
				OldState->OnExit(NewState);
			}

			if (NewState)
			{
				NewState->OnEnter(OldState);
			}

			CurrentState = NewState;
			OnStateChangedDelegate.Broadcast(OldState, NewState);
		}
	}
}

#undef LOCTEXT_NAMESPACE

