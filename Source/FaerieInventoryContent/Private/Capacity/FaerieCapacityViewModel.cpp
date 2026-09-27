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

void UFaerieCapacityViewModel::OnProxySet(FMassEntityManager& EntityManager)
{
	int32 NewWeight = 0;
	FIntVector NewBounds = FIntVector::ZeroValue;
	float NewEfficiency = 0.f;

	if (ItemProxy.IsValid())
	{
		check(!IsInitialized())
		CreateViewModelEntity(EntityManager, GetViewModelFragmentType());

		const ItemData::FCapacityHelper Helper(&EntityManager, ItemProxy.GetItemInstanceOrInvalid());

		UE_MVVM_SET_PROPERTY_VALUE(HasCapacity, Helper.HasCapacity());

		if (HasCapacity)
		{
			auto&& Capacity = Helper.GetCapacity();
			NewWeight = Capacity.Weight;
			NewBounds = Capacity.Bounds;
			NewEfficiency = Capacity.Efficiency;
		}
	}
	else
	{
		DestroyViewModelEntity(EntityManager);
		UE_MVVM_SET_PROPERTY_VALUE(HasCapacity, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(Weight, NewWeight);
	UE_MVVM_SET_PROPERTY_VALUE(Bounds, NewBounds);
	UE_MVVM_SET_PROPERTY_VALUE(Efficiency, NewEfficiency);
}

void UFaerieCapacityViewModel::OnFieldChange(const FMassEntityManager& EntityManager, const ItemData::FFieldChangePayload& Data)
{
	const ItemData::FCapacityHelper Helper(&EntityManager, ItemProxy.GetItemInstanceOrInvalid());

	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Weight))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Weight, Helper.GetCapacity().Weight);
	}
	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Bounds))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Bounds, Helper.GetCapacity().Bounds);
	}
	if (Data.HasFlag(FFaerieItemCapacity::EFieldFlags::Efficiency))
	{
		UE_MVVM_SET_PROPERTY_VALUE(Efficiency, Helper.GetCapacity().Efficiency);
	}
}