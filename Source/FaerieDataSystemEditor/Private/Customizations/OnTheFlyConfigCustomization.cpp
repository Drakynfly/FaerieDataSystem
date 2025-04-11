// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Customizations/OnTheFlyConfigCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"

TSharedRef<IPropertyTypeCustomization> FOnTheFlyConfigCustomization::MakeInstance()
{
	return MakeShared<FOnTheFlyConfigCustomization>();
}

void FOnTheFlyConfigCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
												   FDetailWidgetRow& HeaderRow,
												   IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FOnTheFlyConfigCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle,
                                                    IDetailChildrenBuilder& StructBuilder,
                                                    IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	auto&& TypeHandle = PropertyHandle->GetChildHandle(0);

	auto&& Row = StructBuilder.AddProperty(TypeHandle.ToSharedRef());

	TSharedPtr<SWidget> NameWidget;
	TSharedPtr<SWidget> ValueWidget;
	Row.GetDefaultWidgets(NameWidget, ValueWidget, true);

	Row.CustomWidget(true)
		.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
		.ValueContent()
			[
				ValueWidget.ToSharedRef()
			];

	Row.ShowPropertyButtons(true);
}