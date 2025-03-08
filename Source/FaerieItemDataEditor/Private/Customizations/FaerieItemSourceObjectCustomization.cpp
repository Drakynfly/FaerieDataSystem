// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemSourceObjectCustomization.h"

#include "DetailWidgetRow.h"
#include "FaerieItemSource.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FFaerieItemSourceObjectCustomization::MakeInstance()
{
	return MakeShared<FFaerieItemSourceObjectCustomization>();
}

void FFaerieItemSourceObjectCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& ObjProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object));

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			ObjProp->CreatePropertyValueWidget()
		];
}
