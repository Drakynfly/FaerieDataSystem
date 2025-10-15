// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_Logic.h"
#include "FaerieItem.h"

bool FFaerieItemMutator_ApplyFirst::Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		if (Child.Get().Apply(Stack, Squirrel))
		{
			return true;
		}
	}
	return false;
}

bool FFaerieItemMutator_ApplyAny::Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child.Get().Apply(Stack, Squirrel);
	}
	return true;
}

bool FFaerieItemMutator_ApplyAll::Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	if (!Stack.IsValid()) return false;
	if (!Stack.Item->CanMutate()) return false;

	// In order to support the rollback feature of this mutator, we need to create a copy of the item, in case a mutator fails.
	FFaerieItemStack CopyForMutation{
		Stack.Item->CreateDuplicate(),
		Stack.Copies
	};

	// Apply each mutator to the Copy.
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		if (!Child.Get().Apply(CopyForMutation, Squirrel))
		{
			return false;
		}
	}

	// In case of no failure, copy our local version back to the original stack.
	Stack = CopyForMutation;
	return true;
}
