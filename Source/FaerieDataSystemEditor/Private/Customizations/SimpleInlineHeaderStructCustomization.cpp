// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Customizations/SimpleInlineHeaderStructCustomization.h"
#include "DetailWidgetRow.h"

TSharedRef<IPropertyTypeCustomization> FSimpleInlineHeaderStructCustomization::MakeInstance()
{
	return MakeShared<FSimpleInlineHeaderStructCustomization>();
}

void FSimpleInlineHeaderStructCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                      FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			PropertyHandle->GetChildHandle(0).ToSharedRef()->CreatePropertyValueWidget()
		];
}