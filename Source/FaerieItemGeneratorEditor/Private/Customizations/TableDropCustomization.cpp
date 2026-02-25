// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "TableDropCustomization.h"

#include "Generation/FaerieGenerationStructs.h"

#include "DetailWidgetRow.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemSource.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"

namespace Faerie::GeneratorEditor
{
TSharedRef<IPropertyTypeCustomization> FTableDropCustomization::MakeInstance()
{
	return MakeShared<FTableDropCustomization>();
}

void FTableDropCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& AssetProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, Asset));
	auto&& ObjectProp = AssetProp->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object));

	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			ObjectProp->CreatePropertyValueWidget()
		];

	const FSimpleDelegate OnValueChanged = FSimpleDelegate::CreateLambda([&CustomizationUtils]()
	{
		CustomizationUtils.GetPropertyUtilities()->ForceRefresh();
	});

	PropertyHandle->SetOnChildPropertyValueChanged(OnValueChanged);
}

void FTableDropCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& AssetProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, Asset));
	auto&& ObjectProp = AssetProp->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object));

	auto&& SlotsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieTableDrop, StaticResourceSlots));

    UObject* ObjectValue = nullptr;
    if (auto&& SoftObjectProperty = CastField<FSoftObjectProperty>(ObjectProp->GetProperty()))
    {
        void* DropAddress;
        ObjectProp->GetValueData(DropAddress);
    	if (DropAddress)
    	{
    		ObjectValue = SoftObjectProperty->LoadObjectPropertyValue(DropAddress);
    	}
    }

	void* SlotsAddress;
	if (SlotsHandle->GetValueData(SlotsAddress) != FPropertyAccess::Success)
	{
		return;
	}

	if (!IsValid(ObjectValue))
	{
		return;
	}

	// @todo temp
	ChildBuilder.AddProperty(SlotsHandle.ToSharedRef());
	//return;

	bool ShowSlotsProperty = false;

    // Only display the slots property if the asset is a graph that needs them, or a value has already been set.
    if (auto&& SlotInterface = Cast<IFaerieItemSlotInterface>(ObjectValue))
    {
    	const FFaerieItemCraftingSlots Slots = SlotInterface->GetCraftingSlots();
    	if (Slots.RequiredSlots.IsEmpty())
    	{
    		return;
    	}

    	ShowSlotsProperty = true;

    	FScriptMapHelper MapHelper(CastField<FMapProperty>(SlotsHandle->GetProperty()), SlotsAddress);

    	for (auto&& Slot : Slots.RequiredSlots)
    	{
    		MapHelper.FindOrAdd(&Slot.Name);
    		MapHelper.Rehash();
    	}
    }
	else // Not a crafting asset
	{
		uint32 NumItems;
		SlotsHandle->AsMap()->GetNumElements(NumItems);

		// If there are items in the map, we should still display the property
		if (NumItems > 0)
		{
			ShowSlotsProperty = true;
		}
	}

	if (ShowSlotsProperty)
	{
		ChildBuilder.AddProperty(SlotsHandle.ToSharedRef());
	}
}
}