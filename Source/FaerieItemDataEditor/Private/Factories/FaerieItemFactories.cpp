// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemFactories.h"
#include "ContentBrowserModule.h"
#include "FaerieItemAsset.h"
#include "FaerieItemTemplate.h"
#include "IContentBrowserSingleton.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemFactories)

#define LOCTEXT_NAMESPACE "FaerieItemFactories"

namespace Faerie::Editor
{
    BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

	class SItemAssetCreateDialog final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SItemAssetCreateDialog) {}
		SLATE_END_ARGS()

		/** Constructs this widget with InArgs */
		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SBorder)
					.Visibility(EVisibility::Visible)
					.BorderImage(FAppStyle::GetBrush("Menu.Background"))
					[
						SNew(SBox)
							.Visibility(EVisibility::Visible)
							.WidthOverride(500.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
									.FillHeight(1)
									[
										SNew(SBorder)
											.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
											.Content()
											[
												SAssignNew(TemplateAssetContainer, SVerticalBox)
											]
									]
								+ SVerticalBox::Slot()
									.AutoHeight()
									.HAlign(HAlign_Right)
									.VAlign(VAlign_Bottom)
									.Padding(8)
									[
										SNew(SUniformGridPanel)
											.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
											.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
											.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
										+ SUniformGridPanel::Slot(0, 0)
										[
											SNew(SButton)
												.HAlign(HAlign_Center)
												.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
												.ButtonColorAndOpacity(GColorList.Aquamarine)
												.OnClicked(this, &SItemAssetCreateDialog::CreateEmptyClicked)
												.Text(LOCTEXT("CreateBlankItemAsset", "Create Blank Asset"))
												.ToolTipText(LOCTEXT("CreateBlankItemAssetTooltip", "Create a blank asset with no tokens."))
										]
										+ SUniformGridPanel::Slot(1, 0)
										[
											SNew(SButton)
												.HAlign(HAlign_Center)
												.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
												.ButtonColorAndOpacity(GColorList.BlueViolet)
												.IsEnabled(this, &SItemAssetCreateDialog::CanClickOk)
												.OnClicked(this, &SItemAssetCreateDialog::OkClicked)
												.Text(LOCTEXT("CreateItemAssetOk", "OK"))
												.ToolTipText(LOCTEXT("CreateItemAssetOkTooltip", "Create a new asset from the selected template."))
										]
									]
							]
					]
			];

			MakeParentClassPicker();
		}

		/** Sets properties for the supplied UFaerieItemAsset_Factory */
		bool ConfigureProperties(const TWeakObjectPtr<UFaerieItemAsset_Factory> InFactory)
		{
			FaerieItemAssetFactory = InFactory;

			const TSharedRef<SWindow> Window = SNew(SWindow)
				.Title(LOCTEXT("CreateItemAssetOptions", "Pick Asset Template"))
				.ClientSize(FVector2D(400, 700))
				.SupportsMinimize(false).SupportsMaximize(false)
				[
					AsShared()
				];

			PickerWindow = Window;
			GEditor->EditorAddModalWindow(Window);

			FaerieItemAssetFactory.Reset();
			return bOkClicked;
		}

	private:
		/** Creates the combo menu for the parent class */
		void MakeParentClassPicker()
		{
			// Load the content browser module to display an asset picker
			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

			FAssetPickerConfig AssetPickerConfig;
			AssetPickerConfig.SelectionMode = ESelectionMode::Single;
			AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
			AssetPickerConfig.bAllowNullSelection = false;
			AssetPickerConfig.bAllowDragging = false;

			/** The asset picker will only show assets enabled as templates */
			AssetPickerConfig.Filter.ClassPaths.Add(UFaerieItemAsset::StaticClass()->GetClassPathName());
			AssetPickerConfig.Filter.bRecursiveClasses = true;
			AssetPickerConfig.Filter.TagsAndValues.Add(FName{"IsEditorTemplate"}, FString("true"));

			/** The delegate that fires when an asset was selected */
			AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &SItemAssetCreateDialog::OnTemplatePicked);
			AssetPickerConfig.InitialAssetSelection = nullptr;

			TemplateAssetContainer->ClearChildren();
			TemplateAssetContainer->AddSlot()
				[
					ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
				];
		}

		/** Handler for when a parent class is selected */
		void OnTemplatePicked(const FAssetData& AssetData)
		{
			SelectedTemplate = Cast<UFaerieItemAsset>(AssetData.GetAsset());
		}

    	/** Handler for when create empty is clicked */
    	FReply CreateEmptyClicked()
		{
			if (FaerieItemAssetFactory.IsValid())
			{
				FaerieItemAssetFactory->Template = nullptr;
			}

			CloseDialog(true);

			return FReply::Handled();
		}

		/** Handler for when ok is clicked */
		FReply OkClicked()
		{
			if (FaerieItemAssetFactory.IsValid())
			{
				FaerieItemAssetFactory->Template = SelectedTemplate.Get();
			}

			CloseDialog(true);

			return FReply::Handled();
		}

    	/** Handler for when ok is clicked */
    	bool CanClickOk() const
		{
			return SelectedTemplate.IsValid() && FaerieItemAssetFactory.IsValid();
		}

		void CloseDialog(const bool bWasPicked = false)
		{
			bOkClicked = bWasPicked;
			if (PickerWindow.IsValid())
			{
				PickerWindow.Pin()->RequestDestroyWindow();
			}
		}

		virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::Escape)
			{
				CloseDialog();
				return FReply::Handled();
			}
			return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
		}

	private:
		/** The factory for which we are setting up properties */
		TWeakObjectPtr<UFaerieItemAsset_Factory> FaerieItemAssetFactory;

		/** A pointer to the window that is asking the user to select a parent class */
		TWeakPtr<SWindow> PickerWindow;

		/** The container for the Template Asset picker */
		TSharedPtr<SVerticalBox> TemplateAssetContainer;

		/** The selected class */
		TWeakObjectPtr<UFaerieItemAsset> SelectedTemplate = nullptr;

		/** True if Ok was clicked */
		bool bOkClicked = false;
	};

	END_SLATE_FUNCTION_BUILD_OPTIMIZATION
}

// FAERIE ITEM ASSET
UFaerieItemAsset_Factory::UFaerieItemAsset_Factory(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
    bEditAfterNew = true;
    bCreateNew = true;
    SupportedClass = UFaerieItemAsset::StaticClass();
}

bool UFaerieItemAsset_Factory::ConfigureProperties()
{
    const TSharedRef<Faerie::Editor::SItemAssetCreateDialog> Dialog = SNew(Faerie::Editor::SItemAssetCreateDialog);
    return Dialog->ConfigureProperties(this);
}

UObject* UFaerieItemAsset_Factory::FactoryCreateNew(UClass* Class,
                                                    UObject* InParent,
                                                    const FName Name,
                                                    const EObjectFlags Flags,
                                                    UObject* Context,
                                                    FFeedbackContext* Warn)
{
	if (!IsValid(Context))
	{
		Context = Template;
	}

    return NewObject<UFaerieItemAsset>(InParent, Class, Name, Flags | RF_Transactional, Context);
}

// FAERIE ITEM TEMPLATE
UFaerieItemTemplate_Factory::UFaerieItemTemplate_Factory(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
    bEditAfterNew = true;
    bCreateNew = true;
    SupportedClass = UFaerieItemTemplate::StaticClass();
}

UObject* UFaerieItemTemplate_Factory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
    UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UFaerieItemTemplate>(InParent, Class, Name, Flags | RF_Transactional, Context);
}

#undef LOCTEXT_NAMESPACE