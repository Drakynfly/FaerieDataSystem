// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "IPropertyTypeCustomization.h"

class SWidget;
class STextBlock;

class FInventoryWeightCustomization final : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils) override {}

    void CreateWeightHelp();
    void UpdateWeightHelp() const;

private:
    TSharedPtr<IPropertyHandle> WeightHandlePtr;
    TSharedPtr<STextBlock> WeightHelpText;
};

class FItemCapacityCustomization final : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    void UpdateInfo();

private:
    TSharedPtr<IPropertyHandle> WeightHandle;
    TSharedPtr<IPropertyHandle> BoundsHandle;
    TSharedPtr<IPropertyHandle> EfficiencyHandle;

    TSharedPtr<STextBlock> InfoText;
    TSharedPtr<SWidget> InfoBlock;
};