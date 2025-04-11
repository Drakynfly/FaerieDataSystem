// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "TableDropCustomization.h"

#include "DetailWidgetRow.h"
#include "FaerieItemSlotInterface.h"
#include "GenerationStructs.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertyHandle.h"
#include "Algo/ForEach.h"

TSharedRef<IPropertyTypeCustomization> FTableDropCustomization::MakeInstance()
{
	return MakeShared<FTableDropCustomization>();
}

void FTableDropCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto&& AssetProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTableDrop, Asset));
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
	auto&& AssetProp = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTableDrop, Asset));
	auto&& ObjectProp = AssetProp->GetChildHandle(GET_MEMBER_NAME_CHECKED(FFaerieItemSourceObject, Object));

	auto&& SlotsHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTableDrop, StaticResourceSlots));

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

	TMap<FFaerieItemSlotHandle, TInstancedStruct<FTableDrop>>* SlotsAddress;
	SlotsHandle->GetValueData(reinterpret_cast<void*&>(SlotsAddress));

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
    	const FFaerieCraftingSlotsView SlotsView = Faerie::Crafting::GetCraftingSlots(SlotInterface);
    	const FFaerieItemCraftingSlots& SlotsPtr = SlotsView.Get();

    	if (SlotsPtr.RequiredSlots.IsEmpty() &&
    		SlotsPtr.OptionalSlots.IsEmpty())
    	{
    		return;
    	}

    	ShowSlotsProperty = true;

    	FScriptMapHelper MapHelper(CastField<FMapProperty>(SlotsHandle->GetProperty()), SlotsAddress);

    	// Prefill the map with the slots from the asset:
    	auto SlotIter = [&](const TPair<FFaerieItemSlotHandle, TObjectPtr<UFaerieItemTemplate>>& Slot)
    		{
    			MapHelper.FindOrAdd(&Slot.Key);
    			MapHelper.Rehash();
    		};

    	Algo::ForEach(SlotsPtr.RequiredSlots, SlotIter);
    	Algo::ForEach(SlotsPtr.OptionalSlots, SlotIter);
    }
	else // Not a crafting asset
	{
		uint32 NumItems;
		SlotsHandle->AsArray()->GetNumElements(NumItems);

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