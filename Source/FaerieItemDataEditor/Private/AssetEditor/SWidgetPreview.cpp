// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "SWidgetPreview.h"
#include "ObjectEditorUtils.h"
#include "WidgetBlueprint.h"
#include "AssetEditor/FaerieItemAssetEditor.h"
#include "AssetEditor/FaerieWidgetPreview.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Slate/SRetainerWidget.h"
#include "Widgets/FaerieCardBase.h"

#define LOCTEXT_NAMESPACE "WidgetPreviewStatus"

namespace Faerie::UMGWidgetPreview
{
	struct FWidgetTypeTuple
	{
		FWidgetTypeTuple() = default;

		explicit FWidgetTypeTuple(const UUserWidget* InUserWidgetCDO)
			: bIsValid(true)
		{
			Set(InUserWidgetCDO);
		}

		/** Attempt to resolve the tuple from the given UserWidget CDO. */
		void Set(const UUserWidget* InUserWidgetCDO)
		{
			check(InUserWidgetCDO);

			ClassDefaultObject = InUserWidgetCDO;
			BlueprintGeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(ClassDefaultObject->GetClass());
			Blueprint = Cast<UWidgetBlueprint>(BlueprintGeneratedClass->ClassGeneratedBy);
		}

		const UUserWidget* ClassDefaultObject = nullptr;
		UWidgetBlueprint* Blueprint = nullptr;
		UWidgetBlueprintGeneratedClass* BlueprintGeneratedClass = nullptr;

		/** Returns true if at least one of the tuple values is valid. */
		bool IsValid() const
		{
			return bIsValid && (ClassDefaultObject || Blueprint || BlueprintGeneratedClass);
		}

	private:
		/** True only when value resolution has been attempted. */
		bool bIsValid = false;
	};

	EFixApplicability FWidgetPreviewabilityFixer::GetApplicability(int32 FixIndex) const
	{
		if (const UUserWidget* UserWidget = WeakUserWidget.Get())
		{
			const FWidgetTypeTuple WidgetTuple(UserWidget);
			if (WidgetTuple.BlueprintGeneratedClass)
			{
				return WidgetTuple.BlueprintGeneratedClass->bCanCallInitializedWithoutPlayerContext
					? EFixApplicability::Applied
					: EFixApplicability::CanBeApplied;
			}
		}

		return EFixApplicability::DidNotApply;
	}

	FFixResult FWidgetPreviewabilityFixer::ApplyFix(int32 FixIndex)
	{
		// @todo: apply recursively (named slots, etc.)
		if (const UUserWidget* UserWidget = WeakUserWidget.Get())
		{
			FWidgetTypeTuple WidgetTuple(UserWidget);
			if (UWidgetBlueprint* WidgetBlueprint = WidgetTuple.Blueprint)
			{
				FScopedTransaction Transaction(LOCTEXT("FixWidgetBlueprint", "Fix Widget Blueprint"));

				// Set flag
				{
					FObjectEditorUtils::SetPropertyValue(
						WidgetBlueprint,
						GET_MEMBER_NAME_CHECKED(UWidgetBlueprint, bCanCallInitializedWithoutPlayerContext),
						true);
				}

				// Compile
				{
					FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint, EBlueprintCompileOptions::SkipGarbageCollection);
					WidgetBlueprint->PostEditChange();
					WidgetBlueprint->MarkPackageDirty();
				}

				return FFixResult::Success();
			}
		}

		return FFixResult::Failure(LOCTEXT("FixWidgetBlueprint_Failure", "Failed to fix UserWidget."));
	}

	TSharedRef<FWidgetPreviewabilityFixer> FWidgetPreviewabilityFixer::Create(const UUserWidget* InUserWidget)
	{
		TSharedRef<FWidgetPreviewabilityFixer> Fixer = MakeShared<FWidgetPreviewabilityFixer>();
		Fixer->WeakUserWidget = InUserWidget;
		return Fixer;
	}

	FWidgetPreviewToolkitStateBase::FWidgetPreviewToolkitStateBase(const FName& Id)
		: Id(Id)
	{
	}

	FName FWidgetPreviewToolkitStateBase::GetId() const
	{
		return Id;
	}

	const TSharedPtr<FTokenizedMessage>& FWidgetPreviewToolkitStateBase::GetStatusMessage() const
	{
		return StatusMessage;
	}

	bool FWidgetPreviewToolkitStateBase::CanTick() const
	{
		return bCanTick;
	}

	bool FWidgetPreviewToolkitStateBase::ShouldOverlayStatusMessage() const
	{
		return bShouldOverlayMessage;
	}

	void FWidgetPreviewToolkitStateBase::OnEnter(const FWidgetPreviewToolkitStateBase* InFromState)
	{
		// Default, empty Implementation
	}

	void FWidgetPreviewToolkitStateBase::OnExit(const FWidgetPreviewToolkitStateBase* InToState)
	{
		// Default, empty Implementation
	}

	FWidgetPreviewToolkitPausedState::FWidgetPreviewToolkitPausedState(): FWidgetPreviewToolkitStateBase(TEXT("Paused"))
	{
		StatusMessage = FTokenizedMessage::Create(EMessageSeverity::Info, LOCTEXT("WidgetPreviewToolkitPausedState_Message", "The preview is currently paused."));
		bCanTick = false;
		bShouldOverlayMessage = true;
	}

	FWidgetPreviewToolkitBackgroundState::FWidgetPreviewToolkitBackgroundState()
	{
		Id = TEXT("Background");
		StatusMessage = FTokenizedMessage::Create(EMessageSeverity::Info, LOCTEXT("WidgetPreviewToolkitBackgroundState_Message", "The widget preview is paused while the window is in the background. Re-focus to unpause."));
	}

	FWidgetPreviewToolkitUnsupportedWidgetState::FWidgetPreviewToolkitUnsupportedWidgetState()
	{
		Id = TEXT("UnsupportedWidget");
		ResetStatusMessage();
	}

	void FWidgetPreviewToolkitUnsupportedWidgetState::SetUnsupportedWidgets(const TArray<const UUserWidget*>& InWidgets)
	{
		UnsupportedWidgets.Reset();

		Algo::Transform(
			InWidgets,
			UnsupportedWidgets,
			[](const UUserWidget* InWidget){
				return MakeWeakObjectPtr(InWidget);
			});

		// Reset message
		ResetStatusMessage();

		for (const TWeakObjectPtr<const UUserWidget>& WeakUnsupportedWidget : UnsupportedWidgets)
		{
			if (const UUserWidget* UnsupportedWidget = WeakUnsupportedWidget.Get())
			{
				const TSharedRef<FWidgetPreviewabilityFixer> WidgetFixer = FWidgetPreviewabilityFixer::Create(UnsupportedWidget);

				StatusMessage->AddToken(FAssetNameToken::Create(UnsupportedWidget->GetPackage()->GetName()));
				StatusMessage->AddToken(WidgetFixer->CreateToken(LOCTEXT("FixUnsupportedWidget", "Fix")));
			}
		}
	}

	void FWidgetPreviewToolkitUnsupportedWidgetState::ResetStatusMessage()
	{
		StatusMessage = FTokenizedMessage::Create(EMessageSeverity::Error, LOCTEXT("WidgetPreviewToolkitUnsupportedWidgetState_Message", "One or more referenced widgets isn't supported (\"Can Call Initialized Without Player Context\" might be disabled)."));
	}

	FWidgetPreviewToolkitRunningState::FWidgetPreviewToolkitRunningState(): FWidgetPreviewToolkitStateBase(TEXT("Running"))
	{
		StatusMessage = FTokenizedMessage::Create(EMessageSeverity::Info, LOCTEXT("WidgetPreviewToolkitRunningState_Message", "The preview is running!"));
		bCanTick = true;
		bShouldOverlayMessage = false;
	}

	void SWidgetPreview::Construct(const FArguments& Args, const TSharedRef<FFaerieItemAssetEditor>& InToolkit)
	{
		WeakToolkit = InToolkit;

		OnStateChangedHandle = InToolkit->OnStateChanged().AddSP(this, &SWidgetPreview::OnStateChanged);
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

					if (UUserWidget* PreviewWidget = Preview->GetOrCreateWidgetInstance(World))
					{
						if (UFaerieCardBase* Card = Cast<UFaerieCardBase>(PreviewWidget))
						{
							// Allow blueprint code to run here.
							FEditorScriptExecutionGuard EditorScriptGuard;
							Card->SetItemData(FFaerieItemProxy(Preview), true);
						}
					}

					if (const TSharedPtr<SWidget> PreviewSlateWidget = Preview->GetSlateWidgetInstance())
					{
						CreatedSlateWidget = PreviewSlateWidget;
					}
					else if (UUserWidget* PreviewWidget = Preview->GetOrCreateWidgetInstance(World))
					{
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

#undef LOCTEXT_NAMESPACE