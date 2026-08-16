// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_BlueprintBase.h"
#include "FaerieItemDataView.h"
#include "FaerieItemProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_BlueprintBase)

FAERIE_MUTATOR_IMPL(FFaerieItemMutator_Blueprint)

void FFaerieItemMutator_Blueprint::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	if (IsValid(Blueprint))
	{
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->GetRequiredAssets(RequiredAssets);
	}
}

bool FFaerieItemMutator_Blueprint::Apply(FFaerieItemInstance& Item, const FFaerieItemMutatorContext& Context) const
{
	if (IsValid(Blueprint))
	{
		const Faerie::ItemData::FScopeProxy Proxy(Item, 1, nullptr);
		return GetDefault<UFaerieItemMutator_BlueprintBase>(Blueprint)->Apply(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), Context.Squirrel);
	}
	return false;
}

void UFaerieItemMutator_BlueprintBase::GetRequiredAssets_Implementation(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}

bool UFaerieItemMutator_BlueprintBase::Apply_Implementation(const FFaerieItemProxy& Proxy, USquirrel* Squirrel) const
{
	return false;
}