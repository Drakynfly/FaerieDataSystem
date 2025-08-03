// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

// @NOTE: This class is adapted from UMGWidgetPreview/Source/UMGWidgetPreview/Private/Widgets/SWidgetPreview.h
// as it is private, and cannot be used outside its module. As Epic continues to develop the UMGWidgetPreview plugin in
// future engine versions, I will hopefully be able to remove this, and use the built-in classes.

#include "Misc/DataValidation/Fixer.h"
#include "Widgets/SCompoundWidget.h"

class SBorder;
class UUserWidget;

namespace Faerie::Ed
{
	class FItemAssetEditorToolkit;
}

enum class EWidgetPreviewWidgetChangeType : uint8;

namespace Faerie::UMGWidgetPreview
{
	struct FWidgetPreviewabilityFixer : UE::DataValidation::IFixer
	{
		virtual EFixApplicability GetApplicability(int32 FixIndex) const override;
		virtual FFixResult ApplyFix(int32 FixIndex) override;

		TWeakObjectPtr<const UUserWidget> WeakUserWidget;

		static TSharedRef<FWidgetPreviewabilityFixer> Create(const UUserWidget* InUserWidget);
	};

	struct FWidgetPreviewToolkitStateBase
	{
		explicit FWidgetPreviewToolkitStateBase(const FName& Id);

		virtual ~FWidgetPreviewToolkitStateBase() = default;

		FName GetId() const;
		const TSharedPtr<FTokenizedMessage>& GetStatusMessage() const;
		bool CanTick() const;
		bool ShouldOverlayStatusMessage() const;

		virtual void OnEnter(const FWidgetPreviewToolkitStateBase* InFromState);
		virtual void OnExit(const FWidgetPreviewToolkitStateBase* InToState);

	protected:
		FName Id;
		TSharedPtr<FTokenizedMessage> StatusMessage;
		bool bCanTick = false;
		bool bShouldOverlayMessage = false;
	};

	struct FWidgetPreviewToolkitPausedState : FWidgetPreviewToolkitStateBase
	{
		FWidgetPreviewToolkitPausedState();
	};

	struct FWidgetPreviewToolkitBackgroundState : FWidgetPreviewToolkitPausedState
	{
		FWidgetPreviewToolkitBackgroundState();
	};

	struct FWidgetPreviewToolkitUnsupportedWidgetState : FWidgetPreviewToolkitPausedState
	{
		FWidgetPreviewToolkitUnsupportedWidgetState();

		void SetUnsupportedWidgets(const TArray<const UUserWidget*>& InWidgets);

	private:
		void ResetStatusMessage();

	private:
		TArray<TWeakObjectPtr<const UUserWidget>> UnsupportedWidgets;
	};

	struct FWidgetPreviewToolkitRunningState : FWidgetPreviewToolkitStateBase
	{
		FWidgetPreviewToolkitRunningState();
	};

	class SWidgetPreview
		: public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SWidgetPreview) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& Args, const TSharedRef<Ed::FItemAssetEditorToolkit>& InToolkit);

		virtual ~SWidgetPreview() override;

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	private:
		void OnStateChanged(FWidgetPreviewToolkitStateBase* InOldState, FWidgetPreviewToolkitStateBase* InNewState);
		void OnWidgetChanged(const EWidgetPreviewWidgetChangeType InChangeType);

		/** Convenience method to get world from the associated viewport. */
		UWorld* GetWorld() const;

		TSharedRef<SWidget> GetCreatedSlateWidget() const;

	private:
		TWeakPtr<Ed::FItemAssetEditorToolkit> WeakToolkit;

		TSharedPtr<SRetainerWidget> RetainerWidget;
		TSharedPtr<SBorder> ContainerWidget;
		TWeakPtr<SWidget> CreatedSlateWidget;

		bool bClearWidgetOnNextPaint = false;
		bool bIsRetainedRender = false;

		FDelegateHandle OnStateChangedHandle;
		FDelegateHandle OnWidgetChangedHandle;
	};
}