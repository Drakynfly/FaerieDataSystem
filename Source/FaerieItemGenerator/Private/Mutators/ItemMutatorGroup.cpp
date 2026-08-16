// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutatorGroup.h"
#include "FaerieItemDataView.h"

#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutatorGroup)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemMutatorValidation"

FAERIE_MUTATOR_IMPL(FFaerieItemMutatorGroup)

bool FFaerieItemMutatorGroup::IsDataValid(FDataValidationContext& Context) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid())
		{
			Context.AddError(LOCTEXT("InvalidMutatorInGroup", "Child in Mutator Group is invalid!"));
			return false;
		}
	}
	return true;
}

#undef LOCTEXT_NAMESPACE

#endif

void FFaerieItemMutatorGroup::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	switch (Policy)
	{
	case EFaerieItemMutatorGroupPolicy::ApplyFirst:
		break;
	case EFaerieItemMutatorGroupPolicy::ApplyAny:
		break;
	}

	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child->GetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutatorGroup::Apply(FFaerieItemInstance& Item, const FFaerieItemMutatorContext& Context) const
{
	if (Children.IsEmpty())
	{
		return true;
	}

	switch (Policy)
	{
	case EFaerieItemMutatorGroupPolicy::ApplyFirst:
		{
			for (auto&& Child : Children)
			{
				if (!Child.IsValid()) continue;
				if (Child->Apply(Item, Context))
				{
					return true;
				}
			}
			return false;
		}
		break;
	case EFaerieItemMutatorGroupPolicy::ApplyAny:
		{
			for (auto&& Child : Children)
			{
				if (!Child.IsValid()) continue;
				(void)Child->Apply(Item, Context);
			}
			return true;
		}
		break;
	}

	return false;
}