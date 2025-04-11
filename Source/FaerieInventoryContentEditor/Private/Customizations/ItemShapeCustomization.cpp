// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemShapeCustomization.h"

#include "DetailWidgetRow.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "IDetailChildrenBuilder.h"
#include "SpatialTypes.h"

#define LOCTEXT_NAMESPACE "ItemShapeCustomization"

TSharedRef<IPropertyTypeCustomization> FItemShapeCustomization::MakeInstance()
{
	return MakeShared<FItemShapeCustomization>();
}

void FItemShapeCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
											FDetailWidgetRow& HeaderRow,
											IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
}

void FItemShapeCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle,
												IDetailChildrenBuilder& ChildBuilder,
												IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
	StructHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FItemShapeCustomization::UpdateGridPanel));

	GridPanel = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(1.f));

	UpdateGridPanel();

	ChildBuilder.AddCustomRow(LOCTEXT("GridRow", "Grid"))
				.WholeRowContent()
	[
		SNew(SBox)
		.Padding(FMargin(0.f, 5.f))
		.HAlign(HAlign_Center)
		[
			GridPanel.ToSharedRef()
		]
	];
}

void FItemShapeCustomization::UpdateGridPanel()
{
	static constexpr float CellSize = 24.0f;
	static constexpr float BorderThickness = 1.0f;
	static constexpr float Padding = 0.25f; // This is the padding for the button to show the border color

	GridPanel->ClearChildren();

	// Set the grid panel's slot padding to 0 to avoid gaps
	GridPanel->SetSlotPadding(FMargin(0.f));

	static FButtonStyle ButtonStyle = []()
	{
		FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton");
		FSlateBrush ButtonBrush;
		ButtonBrush.DrawAs = ESlateBrushDrawType::Box;
		ButtonBrush.TintColor = FLinearColor::White;

		ButtonBrush.OutlineSettings.Width = BorderThickness;
		ButtonBrush.OutlineSettings.Color = FLinearColor::Black;
		Style.SetNormal(ButtonBrush);

		ButtonBrush.TintColor = FLinearColor(0.0f, 0.4f, 0.9f, 0.85f);
		Style.SetHovered(ButtonBrush);
		return Style;
	}();

	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			const FIntPoint CellCoord(X, Y);
			GridPanel->AddSlot(X, Y)
			[
				SNew(SBox)
				.HeightOverride(CellSize)
				.WidthOverride(CellSize)
				.Padding(0.f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("Border"))
					.BorderBackgroundColor(FLinearColor::Black)
					.Padding(Padding)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SButton)
						.ButtonStyle(&ButtonStyle)
						.IsEnabled(!StructHandle->IsEditConst())
						.OnClicked_Lambda([this, CellCoord]()
						{
							OnCellClicked(CellCoord);
							return FReply::Handled();
						})
						.ButtonColorAndOpacity_Lambda([this, CellCoord]()
						{
							return IsCellSelected(CellCoord)
										? FLinearColor(0.2f, 0.8f, 0.3f)
										: FLinearColor(0.15f, 0.15f, 0.15f);
						})
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
					]
				]
			];
		}
	}
}


void FItemShapeCustomization::OnGridSizeChanged(const int32 NewValue, ETextCommit::Type CommitType)
{
	GridSize = NewValue;
	ON_SCOPE_EXIT
	{
		UpdateGridPanel();
	};

	if (!StructHandle.IsValid())
	{
		return;
	}

	void* StructPtr = nullptr;
	const FPropertyAccess::Result Result = StructHandle->GetValueData(StructPtr);
	if (Result == FPropertyAccess::Success)
	{
		check(StructPtr);
		FFaerieGridShape* ShapeGrid = static_cast<FFaerieGridShape*>(StructPtr);

		ShapeGrid->Points.RemoveAll(
			[this](const FIntPoint& Point)
			{
				return Point.X >= GridSize || Point.Y >= GridSize;
			});

		StructHandle->NotifyPostChange(EPropertyChangeType::ArrayRemove);
	}
}

void FItemShapeCustomization::OnCellClicked(const FIntPoint CellCoord)
{
	if (!StructHandle.IsValid())
	{
		return;
	}

	void* StructPtr = nullptr;
	const FPropertyAccess::Result Result = StructHandle->GetValueData(StructPtr);
	if (Result == FPropertyAccess::Success)
	{
		check(StructPtr);
		FFaerieGridShape* ShapeGrid = static_cast<FFaerieGridShape*>(StructPtr);

		const FScopedTransaction Transaction(LOCTEXT("TogglePoint", "Toggle point on shape"));

		TArray<UObject*> Outers;
		StructHandle->GetOuterObjects(Outers);
		for (auto&& Outer : Outers)
		{
			Outer->Modify();
		}

		StructHandle->NotifyPreChange();
		if (ShapeGrid->Points.Contains(CellCoord))
		{
			ShapeGrid->Points.Remove(CellCoord);
			StructHandle->NotifyPostChange(EPropertyChangeType::ArrayRemove);
		}
		else
		{
			ShapeGrid->Points.Add(CellCoord);
			StructHandle->NotifyPostChange(EPropertyChangeType::ArrayAdd);
		}

		// @todo Normalize shape after editing!
	}
}

bool FItemShapeCustomization::IsCellSelected(const FIntPoint CellCoord) const
{
	if (!StructHandle.IsValid())
	{
		return false;
	}

	void* StructPtr = nullptr;
	const FPropertyAccess::Result Result = StructHandle->GetValueData(StructPtr);
	if (Result == FPropertyAccess::Success)
	{
		check(StructPtr);
		const FFaerieGridShape* ShapeGrid = static_cast<FFaerieGridShape*>(StructPtr);
		return ShapeGrid->Points.Contains(CellCoord);
	}
	return false;
}

#undef LOCTEXT_NAMESPACE