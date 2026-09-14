// Copyright Epic Games, Inc. All Rights Reserved.

#include "Customizations/SubScriptStructOfMultiCustomization.h"

#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "SPropertyEditorStruct_COPY.h"
#include "SubScriptStructOfMulti.h"

#include "Templates/SubScriptStructOf.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"

class IDetailChildrenBuilder;

namespace Faerie::Editor
{
	/**
	 * Simulates a struct type property field
	 * Can be used when a property should act like a struct type but it isn't one
	 */
	class SMultiStructPropertyEntryBox : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMultiStructPropertyEntryBox)
			: _MetaStructs()
			, _AllowNone(true)
			, _HideViewOptions(false)
			, _ShowDisplayNames(false)
			, _bExcludeBaseStruct(false)
			, _ShowTreeView(false)
		{}
		/** The meta class that the selected struct must be a child-of (optional) */
		SLATE_ARGUMENT(TArray<const UScriptStruct*>, MetaStructs)
		/** Should we be able to select "None" as a struct? (optional) */
		SLATE_ARGUMENT(bool, AllowNone)
		/** Show the View Options part of the struct picker dialog*/
		SLATE_ARGUMENT(bool, HideViewOptions)
		/** true to show struct display names rather than their native names, false otherwise */
		SLATE_ARGUMENT(bool, ShowDisplayNames)
		/** Should we hide the specified MetaStruct from the selection list and only display its children? */
		SLATE_ARGUMENT(bool, bExcludeBaseStruct)
		/** Show the struct picker as a tree view rather than a list*/
		SLATE_ARGUMENT(bool, ShowTreeView)
		/** Attribute used to get the currently selected struct (required) */
		SLATE_ATTRIBUTE(const UScriptStruct*, SelectedStruct)
		/** Delegate used to set the currently selected struct (required) */
		SLATE_EVENT(FOnSetStruct, OnSetStruct)
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SAssignNew(PropertyEditorStruct, Faerie::Editor::SPropertyEditorStruct)
						.MetaStructs(InArgs._MetaStructs)
						.AllowNone(InArgs._AllowNone)
						.ShowViewOptions(!InArgs._HideViewOptions)
						.ShowDisplayNames(InArgs._ShowDisplayNames)
						.ShowTree(InArgs._ShowTreeView)
						.bExcludeBaseStruct(InArgs._bExcludeBaseStruct)
						.SelectedStruct(InArgs._SelectedStruct)
						.OnSetStruct(InArgs._OnSetStruct)
				]
			];
		}

	private:
		/** The widget used to edit the struct 'property' */
		TSharedPtr<SPropertyEditorStruct> PropertyEditorStruct;
	};

	void FSubScriptStructOfMultiCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
	{
		PropertyHandle = InPropertyHandle;

		const FString& MetaStructName1 = PropertyHandle->GetMetaData("MetaStructs");
		const FString& MetaStructName2 = PropertyHandle->GetMetaData("BaseStructs");
		const bool bAllowNone = !(PropertyHandle->GetMetaDataProperty()->PropertyFlags & CPF_NoClear);
		const bool bShowTreeView = PropertyHandle->HasMetaData("ShowTreeView");
		const bool bHideViewOptions = PropertyHandle->HasMetaData("HideViewOptions");
		const bool bShowDisplayNames = PropertyHandle->HasMetaData("ShowDisplayNames");
		const bool bExcludeBaseStruct = PropertyHandle->HasMetaData("ExcludeBaseStruct") || PropertyHandle->HasMetaData("ExcludeBaseStructs");

		TArray<const UScriptStruct*> MetaScriptStructs;
		if (!MetaStructName1.IsEmpty())
		{
			TArray<const UScriptStruct*> Structs = PropertyCustomizationHelpers::GetStructsFromMetadataString(MetaStructName1);
			if (Structs.Num() >= 1)
			{
				MetaScriptStructs = Structs;
			}
		}
		else if (!MetaStructName2.IsEmpty())
		{
			TArray<const UScriptStruct*> Structs = PropertyCustomizationHelpers::GetStructsFromMetadataString(MetaStructName2);
			if (Structs.Num() >= 1)
			{
				MetaScriptStructs = Structs;
			}
		}

		HeaderRow
		.NameContent()
		[
			InPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(250.0f)
		.MaxDesiredWidth(0.0f)
		[
			SNew(SMultiStructPropertyEntryBox)
				.MetaStructs(MetaScriptStructs)
				.AllowNone(bAllowNone)
				.HideViewOptions(bHideViewOptions)
				.ShowDisplayNames(bShowDisplayNames)
				.ShowTreeView(bShowTreeView)
				.bExcludeBaseStruct(bExcludeBaseStruct)
				.SelectedStruct(this, &FSubScriptStructOfMultiCustomization::HandleGetScriptStruct)
				.OnSetStruct(this, &FSubScriptStructOfMultiCustomization::HandleSetScriptStruct)
		];
	}

	void FSubScriptStructOfMultiCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
	{
	}

	const UScriptStruct* FSubScriptStructOfMultiCustomization::HandleGetScriptStruct() const
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
		{
			check(StructProperty->Struct == TBaseStructure<FSubScriptStructOfMulti>::Get());

			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);

			if (RawData.Num() >= 1 && RawData[0])
			{
				return static_cast<FSubScriptStructOfMulti*>(RawData[0])->Get();
			}
		}
		return nullptr;
	}

	void FSubScriptStructOfMultiCustomization::HandleSetScriptStruct(const UScriptStruct* NewStruct)
	{
		if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
		{
			check(StructProperty->Struct == TBaseStructure<FSubScriptStructOfMulti>::Get());

			FSubScriptStructOfMulti DefaultSubScriptStructOf;

			TArray<void*> RawData;
			PropertyHandle->AccessRawData(RawData);
			FSubScriptStructOfMulti* PreviousValue = RawData.Num() == 1 ? static_cast<FSubScriptStructOfMulti*>(RawData[0]) : &DefaultSubScriptStructOf;

			FSubScriptStructOfMulti NewValue = const_cast<UScriptStruct*>(NewStruct);

			FString TextValue;
			StructProperty->Struct->ExportText(TextValue, &NewValue, PreviousValue, nullptr, EPropertyPortFlags::PPF_None, nullptr);
			ensure(PropertyHandle->SetValueFromFormattedString(TextValue, EPropertyValueSetFlags::DefaultFlags) == FPropertyAccess::Result::Success);
		}
	}
}
