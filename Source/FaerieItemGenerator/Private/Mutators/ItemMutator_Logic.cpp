// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_Logic.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_Logic)

void FFaerieItemMutator_ApplyFirst::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child.Get().GetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_ApplyFirst::Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		if (Child.Get().Apply(Stack, Context))
		{
			return true;
		}
	}
	return false;
}

void FFaerieItemMutator_ApplyAny::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child.Get().GetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_ApplyAny::Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child.Get().Apply(Stack, Context);
	}
	return true;
}

void FFaerieItemMutator_ApplyAll::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	for (auto&& Child : Children)
	{
		if (!Child.IsValid()) continue;
		Child.Get().GetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_ApplyAll::Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const
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
		if (!Child.Get().Apply(CopyForMutation, Context))
		{
			return false;
		}
	}

	// In case of no failure, copy our local version back to the original stack.
	Stack = CopyForMutation;
	return true;
}
