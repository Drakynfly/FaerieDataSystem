﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemCapacityCustomization.h"
#include "FaerieEquipmentEditorSettings.h"
#include "PropertyEditing.h"
#include "Math/UnitConversion.h"
#include "Tokens/FaerieCapacityToken.h"

#define LOCTEXT_NAMESPACE "InventoryWeightCustomization"

TSharedRef<IPropertyTypeCustomization> FInventoryWeightCustomization::MakeInstance()
{
    return MakeShareable(new FInventoryWeightCustomization);
}

void FInventoryWeightCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                    FDetailWidgetRow& HeaderRow,
                                                    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    WeightHandlePtr = PropertyHandle->GetChildHandle(0);

    HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()];

    CreateWeightHelp();

    HeaderRow.ValueContent()
    .MinDesiredWidth(300)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .HAlign(HAlign_Fill)
        [
            WeightHandlePtr->CreatePropertyValueWidget()
        ]
        + SHorizontalBox::Slot()
        .Padding(20.f, 0.f)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Center)
        [
            WeightHelpText.ToSharedRef()
        ]
    ];
}

void FInventoryWeightCustomization::CreateWeightHelp()
{
    WeightHelpText = SNew(STextBlock);
    UpdateWeightHelp();
    FSimpleDelegate Delegate = FSimpleDelegate::CreateSP(this, &FInventoryWeightCustomization::UpdateWeightHelp);
    WeightHandlePtr.Get()->SetOnPropertyValueChanged(Delegate);
}

void FInventoryWeightCustomization::UpdateWeightHelp() const
{
    int32 NewValue;
    WeightHandlePtr.Get()->GetValue(NewValue);

    const double Pounds = FUnitConversion::Convert(static_cast<double>(NewValue), EUnit::Grams, EUnit::Pounds);
    const FString PoundsString = FString::Printf(TEXT("lbs: %.1f"), Pounds);

    if (WeightHelpText.IsValid())
    {
        WeightHelpText->SetText(FText::FromString(PoundsString));
    }
}

TSharedRef<IPropertyTypeCustomization> FItemCapacityCustomization::MakeInstance()
{
    return MakeShareable(new FItemCapacityCustomization);
}

void FItemCapacityCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                  FDetailWidgetRow& HeaderRow,
                                                  IPropertyTypeCustomizationUtils& CustomizationUtils)
{}

void FItemCapacityCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
                                                    IDetailChildrenBuilder& StructBuilder,
                                                    IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    WeightHandle =     StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemCapacity, Weight));
    BoundsHandle =     StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemCapacity, Bounds));
    EfficiencyHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FItemCapacity, Efficiency));

    if (!WeightHandle->IsValidHandle() || !BoundsHandle->IsValidHandle() || !EfficiencyHandle->IsValidHandle()) return;

    StructBuilder.AddProperty(WeightHandle.ToSharedRef());
    StructBuilder.AddProperty(BoundsHandle.ToSharedRef());
    StructBuilder.AddProperty(EfficiencyHandle.ToSharedRef());

    auto&& InfoGroup = StructBuilder.AddGroup("Info", LOCTEXT("CapacityInfoGroup", "Info"));

    InfoText = SNew(STextBlock);

    InfoBlock = SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .Padding(5)
        .VAlign(VAlign_Center)
        [
            InfoText.ToSharedRef()
        ];

    InfoGroup.AddWidgetRow()
        [
            InfoBlock.ToSharedRef()
        ];

    UpdateInfo();

    FSimpleDelegate Delegate;
    Delegate.BindRaw(this, &FItemCapacityCustomization::UpdateInfo);

    WeightHandle->SetOnChildPropertyValueChanged(Delegate);
    BoundsHandle->SetOnChildPropertyValueChanged(Delegate);
    EfficiencyHandle->SetOnPropertyValueChanged(Delegate);
}

void FItemCapacityCustomization::UpdateInfo()
{
    if (InfoText.IsValid() &&
        WeightHandle.IsValid() &&
        BoundsHandle.IsValid() &&
        EfficiencyHandle.IsValid())
    {
        int32 WeightValue = 0;
        FIntVector* BoundsValue = nullptr;
        float EfficiencyValue = 0.f;
        if (auto&& WeightValueProp = WeightHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FWeightEditor, Weight)))
        {
            WeightValueProp->GetValue(WeightValue);
        }
        BoundsHandle->GetValueData(reinterpret_cast<void*&>(BoundsValue));
        EfficiencyHandle->GetValue(EfficiencyValue);

        const float CubicSpace = BoundsValue->X * BoundsValue->Y * BoundsValue->Z;
        const float WeightPerCentimeter = WeightValue / CubicSpace;
        const float SuccessiveWeightPerCentimeter = WeightPerCentimeter * EfficiencyValue;

        FString InfoString = FString::Printf(TEXT("Weight/cm3: %.2f (%.2f)"), WeightPerCentimeter, SuccessiveWeightPerCentimeter);

        auto&& CompareStrings = GetDefault<UFaerieEquipmentEditorSettings>()->GetDebugInfoForCCM(CubicSpace);

        for (const FString& Str : CompareStrings)
        {
            InfoString += "\n";
            InfoString += Str;
        }

        InfoText->SetText(FText::FromString(InfoString));
    }
}

#undef LOCTEXT_NAMESPACE