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

UScriptStruct* UFaerieUsesViewModel::GetViewModelFragmentType() const
{
	return Generation::FUsesViewFragment::StaticStruct();
}

void UFaerieUsesViewModel::OnProxySet(FMassEntityManager& EntityManager)
{
	int32 NewUsesRemaining = 0;
	int32 NewMaxUses = 0;

	if (ItemProxy.IsValid())
	{
		check(!IsInitialized())
		CreateViewModelEntity(EntityManager, GetViewModelFragmentType());

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
		DestroyViewModelEntity(EntityManager);
		UE_MVVM_SET_PROPERTY_VALUE(HasUses, false);
	}

	UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, NewUsesRemaining);
	UE_MVVM_SET_PROPERTY_VALUE(MaxUses, NewMaxUses);
}

void UFaerieUsesViewModel::OnFieldChange(const FMassEntityManager& EntityManager, const ItemData::FFieldChangePayload& Data)
{
	const TConstStructView<FFaerieItemUses> Value =
		ItemData::GetEntityFragmentOrDefault<FFaerieItemUses>(
			&EntityManager,
			ItemProxy.GetItemInstanceOrInvalid());

	if (Data.HasFlag(FFaerieItemUses::EFieldFlags::UsesRemaining))
	{
		UE_MVVM_SET_PROPERTY_VALUE(UsesRemaining, Value->UsesRemaining);
	}
	if (Data.HasFlag(FFaerieItemUses::EFieldFlags::MaxUses))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MaxUses, Value->MaxUses);
	}
}
