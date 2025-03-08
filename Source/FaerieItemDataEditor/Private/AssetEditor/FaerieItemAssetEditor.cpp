// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetEditor/FaerieItemAssetEditor.h"
#include "FaerieAssetEditorCommands.h"
#include "FaerieCardSettings.h"
#include "FaerieItemAsset.h"
#include "FaerieItemDataEditorModule.h"
#include "AssetEditor/FaerieItemAssetPreviewScene.h"
#include "AssetEditor/FaerieItemAssetViewport.h"
#include "AssetEditor/FaerieWidgetPreview.h"
#include "Blueprint/UserWidget.h"
#include "CardTokens/CustomCardClass.h"
#include "Slate/SRetainerWidget.h"
#include "Widgets/FaerieCardBase.h"

#define LOCTEXT_NAMESPACE "FaerieItemAssetEditor"

namespace Faerie::UMGWidgetPreview
{
	void SWidgetPreview::Construct(const FArguments& Args, const TSharedRef<FFaerieItemAssetEditor>& InToolkit)
	{
		WeakToolkit = InToolkit;

		//OnStateChangedHandle = InToolkit->OnStateChanged().AddSP(this, &SWidgetPreview::OnStateChanged);
		OnWidgetChangedHandle = InToolkit->GetPreview()->OnWidgetChanged().AddSP(this, &SWidgetPreview::OnWidgetChanged);

		CreatedSlateWidget = SNullWidget::NullWidget;

		ContainerWidget = SNew(SBorder)
		[
			GetCreatedSlateWidget()
		];

		OnWidgetChanged(EWidgetPreviewWidgetChangeType::Assignment);

		ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(RetainerWidget, SRetainerWidget)
				.RenderOnPhase(false)
				.RenderOnInvalidation(false)
				[
					ContainerWidget.ToSharedRef()
				]
			]
		];
	}

	SWidgetPreview::~SWidgetPreview()
	{
		ContainerWidget->ClearContent();

		if (const TSharedPtr<FFaerieItemAssetEditor> Toolkit = WeakToolkit.Pin())
		{
			//Toolkit->OnStateChanged().Remove(OnStateChangedHandle);

			if (UWidgetPreview* Preview = Toolkit->GetPreview())
			{
				Preview->OnWidgetChanged().Remove(OnWidgetChangedHandle);
			}
		}
	}

	int32 SWidgetPreview::OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const
	{
		const int32 Result = SCompoundWidget::OnPaint(
			Args,
			AllottedGeometry,
			MyCullingRect,
			OutDrawElements,
			LayerId,
			InWidgetStyle,
			bParentEnabled);

		if (bClearWidgetOnNextPaint)
		{
			SWidgetPreview* MutableThis = const_cast<SWidgetPreview*>(this);

			MutableThis->CreatedSlateWidget = SNullWidget::NullWidget;
			ContainerWidget->SetContent(GetCreatedSlateWidget());
			MutableThis->bClearWidgetOnNextPaint = false;
		}

		return Result;
	}

	/*
	void SWidgetPreview::OnStateChanged(FWidgetPreviewToolkitStateBase* InOldState, FWidgetPreviewToolkitStateBase* InNewState)
	{
		const bool bShouldUseLiveWidget = InNewState->CanTick();
		bIsRetainedRender = !bShouldUseLiveWidget;
		bClearWidgetOnNextPaint = bIsRetainedRender;
		RetainerWidget->RequestRender();
		RetainerWidget->SetRetainedRendering(bIsRetainedRender);

		if (bShouldUseLiveWidget)
		{
			OnWidgetChanged(EWidgetPreviewWidgetChangeType::Assignment);
		}
	}
	*/

	void SWidgetPreview::OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType)
	{
		// Disallow widget assignment if retaining (cached thumbnail)
		if (bIsRetainedRender)
		{
			return;
		}

		if (InChangeType != EWidgetPreviewWidgetChangeType::Destroyed)
		{
			if (const TSharedPtr<FFaerieItemAssetEditor> Toolkit = WeakToolkit.Pin())
			{
				if (UFaerieWidgetPreview* Preview = Toolkit->GetPreview())
				{
					UWorld* World = GetWorld();
					if (const TSharedPtr<SWidget> PreviewSlateWidget = Preview->GetSlateWidgetInstance())
					{
						CreatedSlateWidget = PreviewSlateWidget;
					}
					else if (UUserWidget* PreviewWidget = Preview->GetOrCreateWidgetInstance(World))
					{
						if (UFaerieCardBase* Card = Cast<UFaerieCardBase>(PreviewWidget))
						{
							// Allow blueprint code to run here.
							FEditorScriptExecutionGuard EditorScriptGuard;
							Card->SetItemData(FFaerieItemProxy(Preview), true);
						}

						CreatedSlateWidget = PreviewWidget->TakeWidget();
					}
					else
					{
						CreatedSlateWidget = SNullWidget::NullWidget;
					}

					ContainerWidget->SetContent(GetCreatedSlateWidget());
				}
			}
		}
	}

	UWorld* SWidgetPreview::GetWorld() const
	{
		if (const TSharedPtr<FFaerieItemAssetEditor> Toolkit = WeakToolkit.Pin())
		{
			return Toolkit->GetPreviewWorld();
		}

		return nullptr;
	}

	TSharedRef<SWidget> SWidgetPreview::GetCreatedSlateWidget() const
	{
		if (TSharedPtr<SWidget> SlateWidget = CreatedSlateWidget.Pin())
		{
			return SlateWidget.ToSharedRef();
		}

		return SNullWidget::NullWidget;
	}
}


static const FLazyName Faerie_DetailsTab("FaerieItemAssetDetailsTab");
static const FLazyName Faerie_ViewportTab("FaerieItemAssetViewportTab");
static const FLazyName Faerie_PreviewTab("FaerieItemAssetPreviewTab");

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
	WidgetPreviewWidget = SNew(Faerie::UMGWidgetPreview::SWidgetPreview, SharedThis(this));
	
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
			if (const UCustomCardClass* CardToken = Cast<UCustomCardClass>(Element))
			{
				WidgetType.ObjectPath = CardToken->GetCardClass().ToSoftObjectPath();
				break;
			}
		}

		if (WidgetType.ObjectPath.IsNull())
		{
			// @todo temp
			if (auto Default = GetDefault<UFaerieCardSettings>()->DefaultClasses.FindArbitraryElement())
			{
				WidgetType.ObjectPath = Default->Value.ToSoftObjectPath();
			}
		}

		WidgetPreview->SetWidgetType(WidgetType);
	}

	return WidgetPreview;
}

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_Details(const FSpawnTabArgs& Args) const
{
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
}

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args) const
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (MeshViewportWidget.IsValid())
	{
		SpawnedTab->SetContent(MeshViewportWidget.ToSharedRef());
		PreviewScene->RefreshMesh();
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FFaerieItemAssetEditor::SpawnTab_WidgetPreview(const FSpawnTabArgs& Args) const
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab).Label(LOCTEXT("WidgetTab_Title", "Widget"));

	if (WidgetPreviewWidget.IsValid())
	{
		SpawnedTab->SetContent(WidgetPreviewWidget.ToSharedRef());
	}

	return SpawnedTab;
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

#undef LOCTEXT_NAMESPACE