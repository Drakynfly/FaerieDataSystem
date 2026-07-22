// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_BlueprintBase.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_BlueprintBase)

FAERIE_MUTATOR_IMPL(FFaerieItemMutator_Blueprint)

void FFaerieItemMutator_Blueprint::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	if (IsValid(Blueprint))
	{
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->NativeGetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_Blueprint::Apply(Faerie::ItemData::FMutableReference& Item, const FFaerieItemMutatorContext& Context) const
{
	if (IsValid(Blueprint))
	{
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->NativeApply(Item, Context.Squirrel);
	}
	return false;
}

void UFaerieItemMutator_BlueprintBase::NativeGetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	GetRequiredAssets(RequiredAssets);
}

bool UFaerieItemMutator_BlueprintBase::NativeApply(const Faerie::ItemData::FMutableReference& Item, USquirrel* Squirrel) const
{
	return Apply(Item.GetInstance(), Squirrel);
}

void UFaerieItemMutator_BlueprintBase::GetRequiredAssets_Implementation(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}

bool UFaerieItemMutator_BlueprintBase::Apply_Implementation(const FFaerieItemInstance& Instance, USquirrel* Squirrel) const
{
	return false;
}