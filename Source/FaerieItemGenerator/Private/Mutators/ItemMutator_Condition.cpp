// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_Condition.h"
#include "FaerieItemStackView.h"
#include "FaerieItemTemplate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_Condition)

bool FFaerieItemMutator_TemplateCondition::Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	if (!Stack.IsValid()) return false;
	if (!Stack.Item->CanMutate()) return false;
	if (IsValid(ItemTemplate))
	{
		if (!ItemTemplate->TryMatch(Stack))
		{
			// Template failed, cannot apply.
			return false;
		}
	}
	if (Child.IsValid())
	{
		return Child.Get().Apply(Stack, Squirrel);
	}
	return false;
}
