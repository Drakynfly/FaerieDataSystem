// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Capacity/FaerieCapacityViewModel.h"
#include "Capacity/CapacityStructs.h"
#include "Capacity/FaerieCapacityHelper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCapacityViewModel)

using namespace Faerie;

TNotNull<UScriptStruct*> UFaerieCapacityViewModel::GetFragmentType() const
{
	return FFaerieItemCapacity::StaticStruct();
}

UScriptStruct* UFaerieCapacityViewModel::GetViewModelFragmentType() const
{
	return Content::FCapacityViewFragment::StaticStruct();
}

void UFaerieCapacityViewModel::OnProxySet()
{
	int32 NewWeight = 0;
	FIntVector NewBounds = FIntVector::ZeroValue;
	float NewEfficiency = 0.f;

	if (ItemProxy.HasValidStack())
	{
		const ItemData::FCapacityHelper Helper(ItemProxy);

		UE_MVVM_SET_PROPERTY_VALUE(HasCapacity, Helper.HasFragmentValue());

		if (HasCapacity)
		{
			const FFaerieItemCapacity* Capacity = Helper.GetFragmentValue();
			NewWeight = Capacity->Weight;
			NewBounds = Capacity->Bounds;
			NewEfficiency = Capacity->Efficiency;
		}
	}
	else
	{
		UE_MVVM_SET_PROPERTY_VALUE(HasCapacity, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(Weight, NewWeight);
	UE_MVVM_SET_PROPERTY_VALUE(Bounds, NewBounds);
	UE_MVVM_SET_PROPERTY_VALUE(Efficiency, NewEfficiency);
}

void UFaerieCapacityViewModel::OnFieldChange(const ItemData::FFieldChangePayload& Data)
{
	const ItemData::FCapacityHelper Helper(ItemProxy);

	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Weight))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Weight, Helper.GetFragmentValue()->Weight);
	}
	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Bounds))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Bounds, Helper.GetFragmentValue()->Bounds);
	}
	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Efficiency))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Efficiency, Helper.GetFragmentValue()->Efficiency);
	}
}