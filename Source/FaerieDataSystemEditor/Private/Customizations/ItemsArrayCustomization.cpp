﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Customizations/ItemsArrayCustomization.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditing.h"

#define LOCTEXT_NAMESPACE "ItemsArrayCustomization"

TSharedRef<IPropertyTypeCustomization> FItemsArrayCustomization::MakeInstance()
{
	return MakeShareable(new FItemsArrayCustomization);
}

void FItemsArrayCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                   FDetailWidgetRow& HeaderRow,
                                                   IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& ArrayHandle = PropertyHandle->GetChildHandle(0);

	HeaderRow.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		];

	HeaderRow.ValueContent()
		[
			ArrayHandle->CreatePropertyValueWidget()
		];
}

void FItemsArrayCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
                                                     IDetailChildrenBuilder& StructBuilder,
                                                     IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> ArrayHandle = StructPropertyHandle->GetChildHandle(0);

	check(ArrayHandle.IsValid())

	bool InGenerateHeader = false;
	bool InDisplayResetToDefault = true;
	bool InDisplayElementNum = false;

	TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShareable(
		new FDetailArrayBuilder(ArrayHandle.ToSharedRef(), InGenerateHeader, InDisplayResetToDefault, InDisplayElementNum));

	ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FItemsArrayCustomization::OnGenerateElement));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FItemsArrayCustomization::OnGenerateElement(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex,
												 IDetailChildrenBuilder& ChildrenBuilder)
{
	ChildrenBuilder.AddProperty(ElementProperty);
}

#undef LOCTEXT_NAMESPACE