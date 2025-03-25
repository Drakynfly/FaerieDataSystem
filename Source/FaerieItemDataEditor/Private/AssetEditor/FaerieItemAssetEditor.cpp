// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetEditor/FaerieItemAssetEditor.h"
#include "FaerieAssetEditorCommands.h"
#include "FaerieCardSettings.h"
#include "FaerieItemAsset.h"
#include "FaerieItemCardTags.h"
#include "FaerieItemDataEditorModule.h"
#include "SWidgetPreview.h"
#include "SWidgetPreviewStatus.h"
#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetViewport.h"
#include "AssetEditor/FaerieWidgetPreview.h"
#include "Blueprint/UserWidget.h"
#include "CardTokens/FaerieItemCardToken.h"

#define LOCTEXT_NAMESPACE "FaerieItemAssetEditor"

static const FLazyName Faerie_DetailsTab("FaerieItemAssetDetailsTab");
static const FLazyName Faerie_ViewportTab("FaerieItemAssetViewportTab");
static const FLazyName Faerie_PreviewTab("FaerieItemAssetPreviewTab");

static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitPausedState PausedState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitBackgroundState BackgroundState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitUnsupportedWidgetState UnsupportedWidgetState;
static Faerie::UMGWidgetPreview::FWidgetPreviewToolkitRunningState RunningState;

FFaerieItemAssetEditor::~FFaerieItemAssetEditor()
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

void FFaerieItemAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(INVTEXT("Faerie Item Asset"));

	// Register Details
	InTabManager->RegisterTabSpawner(Faerie_DetailsTab,
		FOnSpawnTab::CreateSP(this, &FFaerieItemAssetEditor::SpawnTab_Details))
				.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	// Register Viewport
	InTabManager->RegisterTabSpawner(Faerie_ViewportTab,
		FOnSpawnTab::CreateSP(this, &FFaerieItemAssetEditor::SpawnTab_Viewport))
				.SetDisplayName(LOCTEXT("MeshViewport", "Mesh Viewport"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	// Register Preview
	InTabManager->RegisterTabSpawner(Faerie_PreviewTab,
		FOnSpawnTab::CreateSP(this, &FFaerieItemAssetEditor::SpawnTab_WidgetPreview))
				.SetDisplayName(LOCTEXT("WidgetPreviewViewport", "Widget Preview"))
				.SetGroup(WorkspaceMenuCategory.ToSharedRef())
				.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x")); // @todo widget icon!
}

void FFaerieItemAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Faerie_DetailsTab);
	InTabManager->UnregisterTabSpawner(Faerie_ViewportTab);
	InTabManager->UnregisterTabSpawner(Faerie_PreviewTab);
}

void FFaerieItemAssetEditor::PostInitAssetEditor()
{
	FAssetEditorToolkit::PostInitAssetEditor();

	if (GEditor)
	{
		OnBlueprintPrecompileHandle = GEditor->OnBlueprintPreCompile().AddRaw(this, &FFaerieItemAssetEditor::OnBlueprintPrecompile);
	}

	OnWidgetChangedHandle = WidgetPreview->OnWidgetChanged().AddSP(this, &FFaerieItemAssetEditor::OnWidgetChanged);

	if (FSlateApplication::IsInitialized())
	{
		OnFocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddSP(this, &FFaerieItemAssetEditor::OnFocusChanging);
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

void FFaerieItemAssetEditor::SaveAsset_Execute()
{
	FAssetEditorToolkit::SaveAsset_Execute();

	if (PreviewScene.IsValid())
	{
		PreviewScene->RefreshMesh();
	}
}

void FFaerieItemAssetEditor::OnClose()
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

void FFaerieItemAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(WidgetPreview);
}

FString FFaerieItemAssetEditor::GetReferencerName() const
{
	return TEXT("FFaerieItemAssetEditor");
}

void FFaerieItemAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
											  FProperty* PropertyThatChanged)
{
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

	if (PreviewScene.IsValid())
	{
		PreviewScene->RefreshMesh();
	}
}

void FFaerieItemAssetEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent,
	class FEditPropertyChain* PropertyThatChanged)
{
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);

	if (PreviewScene.IsValid())
	{
		PreviewScene->RefreshMesh();
	}
}

void FFaerieItemAssetEditor::InitAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UFaerieItemAsset* InItemAsset)
{
	BindCommands();
	
	ItemAsset = InItemAsset;
	
	// Create viewport widget
	const FFaerieItemAssetViewportRequiredArgs Args(
		CreatePreviewScene(),
		SharedThis(this)
	);

	MeshViewportWidget = SNew(SFaerieItemAssetViewport, Args);

	WidgetPreview = CreateWidgetPreview();
	WidgetPreviewWidget =
		SNew(Faerie::UMGWidgetPreview::SWidgetPreview, SharedThis(this))
		.IsEnabled(this, &FFaerieItemAssetEditor::ShouldUpdate);
	
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

void FFaerieItemAssetEditor::BindCommands()
{
	const Faerie::FAssetEditorCommands& Commands = Faerie::FAssetEditorCommands::Get();
	
	ToolkitCommands->MapAction(Commands.FocusViewport, FExecuteAction::CreateSP(this, &FFaerieItemAssetEditor::FocusViewport));
}

void FFaerieItemAssetEditor::ExtendToolbars()
{
	struct FLocal
	{
		static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
		{
			ToolbarBuilder.BeginSection("ExtendToolbarItem");
			{
				ToolbarBuilder.AddToolBarButton(Faerie::FAssetEditorCommands::Get().FocusViewport,
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

TSharedRef<FFaerieItemAssetPreviewScene> FFaerieItemAssetEditor::CreatePreviewScene()
{
	// Create Preview Scene
	if (!PreviewScene.IsValid())
	{
		PreviewScene = MakeShared<FFaerieItemAssetPreviewScene>(
			FPreviewScene::ConstructionValues()
			.AllowAudioPlayback(true)
			.ShouldSimulatePhysics(true)
			.ForceUseMovementComponentInNonGameWorld(true),
			StaticCastSharedRef<FFaerieItemAssetEditor>(AsShared()));
	}

	return PreviewScene.ToSharedRef();
}

UFaerieWidgetPreview* FFaerieItemAssetEditor::CreateWidgetPreview()
{
	if (!IsValid(WidgetPreview))
	{
		WidgetPreview = NewObject<UFaerieWidgetPreview>();
		WidgetPreview->InitFaerieWidgetPreview(ItemAsset);
		FPreviewableWidgetVariant WidgetType;

		for (auto&& Element : ItemAsset->GetEditorTokensView())
		{
			if (const UFaerieItemCardToken* CardToken = Cast<UFaerieItemCardToken>(Element))
			{
				WidgetType.ObjectPath = CardToken->GetCardClass(Faerie::CardType_Full).ToSoftObjectPath();
				break;
			}
		}

		if (WidgetType.ObjectPath.IsNull())
		{
			// @todo temp
			if (auto Default = GetDefault<UFaerieCardSettings>()->FallbackClasses.Find(Faerie::CardType_Full))
			{
				WidgetType.ObjectPath = Default->ToSoftObjectPath();
			}
		}

		WidgetPreview->SetWidgetType(WidgetType);
	}

	if (WidgetPreview)
	{
		WidgetPreview->GetOrCreateWidgetInstance(GetPreviewWorld(), true);
	}

	return WidgetPreview;
}

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args) const
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

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args) const
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

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_WidgetPreview(const FSpawnTabArgs& Args) const
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("WidgetTab_Title", "Widget"));

	TSharedRef<FFaerieItemAssetEditor, ESPMode::ThreadSafe> Self = ConstCastSharedRef<FFaerieItemAssetEditor>( SharedThis(this) );

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

bool FFaerieItemAssetEditor::ShouldUpdate() const
{
	if (CurrentState)
	{
		return CurrentState->CanTick();
	}

	return bIsFocused;
}

void FFaerieItemAssetEditor::OnBlueprintPrecompile(UBlueprint* InBlueprint)
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

void FFaerieItemAssetEditor::OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType)
{
	ResolveState();
}

void FFaerieItemAssetEditor::OnFocusChanging(
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

void FFaerieItemAssetEditor::ResolveState()
{
	Faerie::UMGWidgetPreview::FWidgetPreviewToolkitStateBase* NewState = nullptr;

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

void FFaerieItemAssetEditor::ResetPreview()
{
	if (WidgetPreview)
	{
		// Don't need the returned instance, just need to have it rebuild
		WidgetPreview->GetOrCreateWidgetInstance(GetPreviewWorld(), true);
	}
}

UWorld* FFaerieItemAssetEditor::GetPreviewWorld() const
{
	if (PreviewScene.IsValid())
	{
		return PreviewScene->GetWorld();
	}
	return nullptr;
}

void FFaerieItemAssetEditor::FocusViewport() const
{
	if (MeshViewportWidget.IsValid())
	{
		MeshViewportWidget->OnFocusViewportToSelection();
	}
}

void FFaerieItemAssetEditor::SetState(Faerie::UMGWidgetPreview::FWidgetPreviewToolkitStateBase* InNewState)
{
	Faerie::UMGWidgetPreview::FWidgetPreviewToolkitStateBase* OldState = CurrentState;
	Faerie::UMGWidgetPreview::FWidgetPreviewToolkitStateBase* NewState = InNewState;

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

#undef LOCTEXT_NAMESPACE

