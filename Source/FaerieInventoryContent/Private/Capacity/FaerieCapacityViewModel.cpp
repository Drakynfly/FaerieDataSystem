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

void UFaerieCapacityViewModel::OnProxySet(const FMassEntityManager& EntityManager)
{
	int32 NewWeight = 0;
	FIntVector NewBounds = FIntVector::ZeroValue;
	float NewEfficiency = 0.f;

	if (ItemProxy.IsValid())
	{
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
		UE_MVVM_SET_PROPERTY_VALUE(HasCapacity, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(Weight, NewWeight);
	UE_MVVM_SET_PROPERTY_VALUE(Bounds, NewBounds);
	UE_MVVM_SET_PROPERTY_VALUE(Efficiency, NewEfficiency);
}

void UFaerieCapacityViewModel::OnFieldChange(const FMassEntityManager& EntityManager, const ItemData::FFieldChange& Data)
{
	const ItemData::FCapacityHelper Helper(&EntityManager, ItemProxy.GetItemInstanceOrInvalid());

	for (auto&& Field : Data.Fields)
	{
		if (Field == GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Weight))
		{
			UE_MVVM_SET_PROPERTY_VALUE(Weight, Helper.GetCapacity().Weight);
		}
		else if (Field == GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Bounds))
		{
			UE_MVVM_SET_PROPERTY_VALUE(Bounds, Helper.GetCapacity().Bounds);
		}
		else if (Field == GET_MEMBER_NAME_CHECKED(FFaerieItemCapacity, Efficiency))
		{
			UE_MVVM_SET_PROPERTY_VALUE(Efficiency, Helper.GetCapacity().Efficiency);
		}
	}
}

void UFaerieCapacityViewModel::CheckForFieldChange(const TValid<const FFaerieItemInstance&> Item,
	const FConstStructView FragmentView)
{
	const FFaerieItemCapacity& MassCapacity = FragmentView.Get<const FFaerieItemCapacity>();

	UE_MVVM_SET_PROPERTY_VALUE(Weight, MassCapacity.Weight);
	UE_MVVM_SET_PROPERTY_VALUE(Bounds, MassCapacity.Bounds);
	UE_MVVM_SET_PROPERTY_VALUE(Efficiency, MassCapacity.Efficiency);
}
