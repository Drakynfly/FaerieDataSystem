// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieUsesViewModel.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieUsesViewModel)

using namespace Faerie;

TNotNull<UScriptStruct*> UFaerieUsesViewModel::GetFragmentType() const
{
	return FFaerieItemUses::StaticStruct();
}

void UFaerieUsesViewModel::OnProxySet(const FMassEntityManager& EntityManager)
{
	int32 NewUsesRemaining = 0;
	int32 NewMaxUses = 0;

	if (ItemProxy.IsValid())
	{
		const TConstStructView<FFaerieItemUses> Value =
			ItemData::GetEntityFragmentOrDefault<FFaerieItemUses>(
				&EntityManager,
				ItemProxy.GetItemInstanceOrInvalid());

		UE_MVVM_SET_PROPERTY_VALUE(HasUses, Value.IsValid());

		if (HasUses)
		{
			NewUsesRemaining = Value->UsesRemaining;
			NewMaxUses = Value->MaxUses;
		}
	}
	else
	{
		UE_MVVM_SET_PROPERTY_VALUE(HasUses, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, NewUsesRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(MaxUses, NewMaxUses);
}

void UFaerieUsesViewModel::OnFieldChange(const FMassEntityManager& EntityManager, const ItemData::FFieldChange& Data)
{
	const TConstStructView<FFaerieItemUses> Value =
		ItemData::GetEntityFragmentOrDefault<FFaerieItemUses>(
			&EntityManager,
			ItemProxy.GetItemInstanceOrInvalid());

	for (auto&& Field : Data.Fields)
	{
		if (Field == GET_MEMBER_NAME_CHECKED(FFaerieItemUses, UsesRemaining))
		{
			UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, Value->UsesRemaining);
		}
		else if (Field == GET_MEMBER_NAME_CHECKED(FFaerieItemUses, MaxUses))
		{
			UE_MVVM_SET_PROPERTY_VALUE(MaxUses, Value->MaxUses);
		}
	}
}

void UFaerieUsesViewModel::CheckForFieldChange(const TValid<const FFaerieItemInstance&> Item,
	const FConstStructView FragmentView)
{
	const FFaerieItemUses& MassCapacity = FragmentView.Get<const FFaerieItemUses>();

	UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, MassCapacity.UsesRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(MaxUses, MassCapacity.MaxUses);
}
