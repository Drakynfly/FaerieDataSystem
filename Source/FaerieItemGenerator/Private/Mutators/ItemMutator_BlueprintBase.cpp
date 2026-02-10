// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_BlueprintBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_BlueprintBase)

void FFaerieItemMutator_Blueprint::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	if (IsValid(Blueprint))
	{
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->NativeGetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_Blueprint::Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const
{
	if (IsValid(Blueprint))
	{
		USquirrel* Squirrel = Context ? Context->Squirrel : nullptr;
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->NativeApply(Stack, Squirrel);
	}
	return false;
}

void UFaerieItemMutator_BlueprintBase::NativeGetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	GetRequiredAssets(RequiredAssets);
}

bool UFaerieItemMutator_BlueprintBase::NativeApply(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	return Apply(Stack, Squirrel);
}

void UFaerieItemMutator_BlueprintBase::GetRequiredAssets_Implementation(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}

bool UFaerieItemMutator_BlueprintBase::Apply_Implementation(FFaerieItemStack& Stack, USquirrel* Squirrel) const
{
	return false;
}