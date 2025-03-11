// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

// @NOTE: This class is adapted from UMGWidgetPreview/Source/UMGWidgetPreview/Private/Widgets/SWidgetPreviewStatus.h
// as it is private, and cannot be used outside its module. As Epic continues to develop the UMGWidgetPreview plugin in
// future engine versions, I will hopefully be able to remove this, and use the built-in classes.

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"

class FFaerieItemAssetEditor;

namespace Faerie::UMGWidgetPreview
{
	struct FWidgetPreviewToolkitStateBase;

	class SWidgetPreviewStatus
		: public SCompoundWidget
	{
		SLATE_BEGIN_ARGS(SWidgetPreviewStatus) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& Args, const TSharedRef<FFaerieItemAssetEditor>& InToolkit);

		virtual ~SWidgetPreviewStatus() override;

	private:
		void OnStateChanged(FWidgetPreviewToolkitStateBase* InOldState, FWidgetPreviewToolkitStateBase* InNewState);

		TSharedRef<SWidget> MakeMessageWidget();

		TSharedPtr<FTokenizedMessage> GetStatusMessage() const;

		EVisibility GetStatusVisibility() const;
		const FSlateBrush* GetSeverityIconBrush() const;
		EMessageSeverity::Type GetSeverity() const;
		FText GetMessage() const;

	private:
		TWeakPtr<FFaerieItemAssetEditor> WeakToolkit;
		TSharedPtr<SBox> MessageContainerWidget;

		FDelegateHandle OnStateChangedHandle;
	};
}
