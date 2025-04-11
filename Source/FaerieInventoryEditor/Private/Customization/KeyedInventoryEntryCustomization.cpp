// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "KeyedInventoryEntryCustomization.h"
#include "InventoryDataStructs.h"

#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "InventoryContentCustomization"

TSharedRef<IPropertyTypeCustomization> FInventoryContentCustomization::MakeInstance()
{
	return MakeShared<FInventoryContentCustomization>();
}

void FInventoryContentCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
												     FDetailWidgetRow& HeaderRow,
												     IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& ArrayHandle = PropertyHandle->GetChildHandle("Items");

	HeaderRow.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		];

	HeaderRow.ValueContent()
		[
			ArrayHandle->CreatePropertyValueWidget()
		];
}

void FInventoryContentCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
													   IDetailChildrenBuilder& StructBuilder,
													   IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> ArrayHandle = StructPropertyHandle->GetChildHandle("Items");

	check(ArrayHandle.IsValid())

	TSharedRef<FDetailArrayBuilder> ArrayBuilder = MakeShared<FDetailArrayBuilder>(
		ArrayHandle.ToSharedRef(), false, true, false);

	ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FInventoryContentCustomization::OnGenerateElement));

	StructBuilder.AddCustomBuilder(ArrayBuilder);
}

void FInventoryContentCustomization::OnGenerateElement(TSharedRef<IPropertyHandle> ElementProperty, int32 ElementIndex,
													   IDetailChildrenBuilder& ChildrenBuilder)
{
	ChildrenBuilder.AddProperty(ElementProperty);
}

TSharedRef<IPropertyTypeCustomization> FKeyedInventoryEntryCustomization::MakeInstance()
{
	return MakeShared<FKeyedInventoryEntryCustomization>();
}

void FKeyedInventoryEntryCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle,
														 FDetailWidgetRow& HeaderRow,
														 IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& KeyHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKeyedInventoryEntry, Key));

	HeaderRow.NameWidget
		[
			PropertyHandle->CreatePropertyNameWidget()
		];

	HeaderRow.ValueWidget
		[
			KeyHandle->CreatePropertyValueWidget()
			//SNew(SProperty, KeyHandle->GetChildHandle(0)).ShouldDisplayName(false)
		];
}

void FKeyedInventoryEntryCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
														   IDetailChildrenBuilder& StructBuilder,
														   IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	auto&& ValueHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FKeyedInventoryEntry, Value));
	auto&& ItemDataHandle = ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInventoryEntry, ItemObject));
	auto&& StacksHandle = ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInventoryEntry, Stacks));
	auto&& LimitHandle = ValueHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FInventoryEntry, Limit));

	StructBuilder.AddProperty(ItemDataHandle.ToSharedRef());
	StructBuilder.AddProperty(StacksHandle.ToSharedRef());
	StructBuilder.AddProperty(LimitHandle.ToSharedRef());
}

TSharedRef<IPropertyTypeCustomization> FInventoryEntryCustomization::MakeInstance()
{
	return MakeShared<FInventoryEntryCustomization>();
}

void FInventoryEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FInventoryEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}

#undef LOCTEXT_NAMESPACE