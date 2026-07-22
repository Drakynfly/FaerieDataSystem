// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Consumable/FaerieUsesViewModel.h"
#include "Consumable/FaerieItemUsesFragment.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieUsesViewModel)

using namespace Faerie;

TNotNull<UScriptStruct*> UFaerieUsesViewModel::GetFragmentType() const
{
	return FFaerieItemUses::StaticStruct();
}

void UFaerieUsesViewModel::OnProxySet()
{
	int32 NewUsesRemaining = 0;
	int32 NewMaxUses = 0;

	if (ItemProxy.IsValid())
	{
		const ItemData::FOptionalEntityManager EntityManager(this);
		const ItemData::FUsesHelper Helper(EntityManager, ItemProxy->GetItemInstance().GetValue());

		UE_MVVM_SET_PROPERTY_VALUE(HasUses, Helper.HasFragmentValue());

		if (HasUses)
		{
			auto&& Uses = Helper.GetFragmentValue();
			NewUsesRemaining = Uses->UsesRemaining;
			NewMaxUses = Uses->MaxUses;
		}
	}
	else
	{
		UE_MVVM_SET_PROPERTY_VALUE(HasUses, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, NewUsesRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(MaxUses, NewMaxUses);
}

void UFaerieUsesViewModel::OnFieldChange(const ItemData::FFieldChange& Data)
{
	const ItemData::FUsesHelper Helper(ItemData::FOptionalEntityManager(this), ItemProxy->GetItemInstance().GetValue());
	const FFaerieItemUses* Value = Helper.GetFragmentValue();

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

void UFaerieUsesViewModel::CheckForFieldChange(const ItemData::FReference& Item,
	const FConstStructView FragmentView)
{
	const FFaerieItemUses& MassCapacity = FragmentView.Get<const FFaerieItemUses>();

	UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, MassCapacity.UsesRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(MaxUses, MassCapacity.MaxUses);
}
