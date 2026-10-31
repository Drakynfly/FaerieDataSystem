// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieUsesViewModel.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "FaerieMassFragmentAccessors.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieUsesViewModel)

using namespace Faerie;

TNotNull<UScriptStruct*> UFaerieUsesViewModel::GetFragmentType() const
{
	return FFaerieItemUses::StaticStruct();
}

UScriptStruct* UFaerieUsesViewModel::GetViewModelFragmentType() const
{
	return Generation::FUsesViewFragment::StaticStruct();
}

void UFaerieUsesViewModel::OnProxySet()
{
	int32 NewUsesRemaining = 0;
	int32 NewMaxUses = 0;

	if (ItemProxy.HasValidStack())
	{
		const TConstStructView<FFaerieItemUses> Value = ItemData::GetItemDataOrDefault<FFaerieItemUses>(ItemProxy);

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

void UFaerieUsesViewModel::OnFieldChange(const ItemData::FFieldChangePayload& Data)
{
	const ItemData::FUsesHelper Helper(ItemProxy);

	if (Data.HasFlag(FFaerieItemUses::EFieldFlags::UsesRemaining))
	{
		UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, Helper.GetFragmentValue()->UsesRemaining);
	}
	if (Data.HasFlag(FFaerieItemUses::EFieldFlags::MaxUses))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MaxUses, Helper.GetFragmentValue()->MaxUses);
	}
}
