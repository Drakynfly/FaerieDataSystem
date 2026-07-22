// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemMutatorCustomization.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "FaerieItemMutator.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyCustomizationHelpers.h"

#define LOCTEXT_NAMESPACE "ItemMutatorCustomization"

namespace Faerie::GeneratorEditor
{
TSharedRef<IPropertyTypeCustomization> FItemMutatorCustomization::MakeInstance()
{
	return MakeShared<FItemMutatorCustomization>();
}

void FItemMutatorCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TSharedPtr<IPropertyUtilities> PropertyUtils = CustomizationUtils.GetPropertyUtilities();

	const UScriptStruct* MutatorType = nullptr;
	void* StructPtr = nullptr;
	const FPropertyAccess::Result Result = PropertyHandle->GetValueData(StructPtr);
	if (Result == FPropertyAccess::Success)
	{
		check(StructPtr);
		FFaerieItemMutator* MutatorValue = static_cast<FFaerieItemMutator*>(StructPtr);
		MutatorType = MutatorValue->GetScriptStruct();
	}

	// Create default name and value widgets, we don't override these.
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				PropertyHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				PropertyCustomizationHelpers::MakeCustomButton(
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search").GetIcon(),
					FSimpleDelegate::CreateLambda([MutatorType]()
					{
						if (IsValid(MutatorType))
						{
							TArray<FAssetIdentifier> AssetIdentifiers;
							AssetIdentifiers.Add(FAssetIdentifier(FFaerieItemMutator::StaticStruct(), *MutatorType->GetName()));
							FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
						}
					}),
					LOCTEXT("OpenStructReferenceTooltip", "View assets that reference this struct type"),
					TAttribute<bool>::CreateRaw(this, &FItemMutatorCustomization::IsOpenReferenceViewBound)
				)
			]
		].IsEnabled(MakeAttributeLambda([=] { return !PropertyHandle->IsEditConst() && PropertyUtils->IsPropertyEditingEnabled(); }));

	if (FEditorDelegates::OnOpenReferenceViewer.IsBound())
	{
		if (IsValid(MutatorType))
		{
			HeaderRow.AddCustomContextMenuAction(FUIAction(
                FExecuteAction::CreateLambda([MutatorType]()
                {
                    TArray<FAssetIdentifier> AssetIdentifiers;
                    AssetIdentifiers.Add(FAssetIdentifier(FFaerieItemMutator::StaticStruct(), *MutatorType->GetName()));
                    FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
                }),
				FCanExecuteAction::CreateLambda([]()
				{
					return true;
				})),

                LOCTEXT("OpenStructReference", "View Struct References"),
                LOCTEXT("OpenStructReferenceTooltip", "View assets that reference this struct type"),
                FSlateIcon());
		}
	}
}

void FItemMutatorCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// @Todo this doesnt add structs correctly...

	uint32 ChildCount;
	PropertyHandle->GetNumChildren(ChildCount);

	for (uint32 i = 0; i < ChildCount; ++i)
	{
		const TSharedPtr<IPropertyHandle> ChildPropertyHandle = PropertyHandle->GetChildHandle(i);
		if (!ChildPropertyHandle->IsCustomized())
		{
			ChildBuilder.AddProperty(ChildPropertyHandle.ToSharedRef());
		}
	}
}

bool FItemMutatorCustomization::IsOpenReferenceViewBound() const
{
	return FEditorDelegates::OnOpenReferenceViewer.IsBound();
}
}

#undef LOCTEXT_NAMESPACE