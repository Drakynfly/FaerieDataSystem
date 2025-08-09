// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "WeightedDropCustomization.h"
#include "Generation/FaerieGenerationStructs.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WeightedDropCustomization"

namespace Faerie
{
TSharedRef<IPropertyTypeCustomization> FWeightedDropCustomization::MakeInstance()
{
    return MakeShared<FWeightedDropCustomization>();
}

void FWeightedDropCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                 FDetailWidgetRow& HeaderRow,
                                                 IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    const TSharedRef<IPropertyHandle> WeightHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieWeightedDrop, Weight)).ToSharedRef();
    const TSharedRef<IPropertyHandle> AdjustedHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieWeightedDrop, AdjustedWeight)).ToSharedRef();
    const TSharedRef<IPropertyHandle> DropHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieWeightedDrop, Drop)).ToSharedRef();
    const TSharedRef<IPropertyHandle> PercentageHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieWeightedDrop, PercentageChanceToDrop)).ToSharedRef();

    const TSharedRef<IPropertyHandle> AssetHandle = DropHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, Asset)).ToSharedRef();
    const TSharedRef<IPropertyHandle> ObjectHandle = AssetHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object)).ToSharedRef();

    if (!WeightHandle->IsValidHandle() || !DropHandle->IsValidHandle()) return;

    HeaderRow
        .NameContent()
        [
            PropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .HAlign(HAlign_Fill)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
                .HAlign(HAlign_Fill)
                .MaxWidth(60.f)
                .Padding(10.f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    [
                        WeightHandle->CreatePropertyValueWidget()
                    ]
                    + SVerticalBox::Slot()
                    [
                        AdjustedHandle->CreatePropertyValueWidget()
                    ]
                    + SVerticalBox::Slot()
                    [
                        SNew(STextBlock)
                        .Justification(ETextJustify::Right)
                        .ToolTipText(LOCTEXT("PercentageTooltip", "Chance percentage for this drop to be chosen"))
                        .Text_Lambda([PercentageHandle]
                        {
                            float Percentage;
                            PercentageHandle->GetValue(Percentage);
                            return FText::FromString(FString::Printf(TEXT("%.2f%%"), Percentage));
                        })
                    ]
                ]
            + SHorizontalBox::Slot()
                .HAlign(HAlign_Fill)
                .MaxWidth(400.f)
                [
                    ObjectHandle->CreatePropertyValueWidget()
                ]
        ];
}

void FWeightedDropCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
                                                   IDetailChildrenBuilder& StructBuilder,
                                                   IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    const TSharedRef<IPropertyHandle> DropHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieWeightedDrop, Drop)).ToSharedRef();
    const TSharedRef<IPropertyHandle> AssetHandle = DropHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, Asset)).ToSharedRef();
    const TSharedRef<IPropertyHandle> SlotsHandle = DropHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, StaticResourceSlots)).ToSharedRef();
    const TSharedRef<IPropertyHandle> ObjectHandle = AssetHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object)).ToSharedRef();

    StructBuilder.AddProperty(SlotsHandle);
}
}

#undef LOCTEXT_NAMESPACE