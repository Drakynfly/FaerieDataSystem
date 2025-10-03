// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMutator.h"
#include "FaerieItem.h"
#include "FaerieItemDataFilter.h"
#include "FaerieItemTemplate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemMutator)

bool UFaerieItemMutator::CanApply(const FFaerieItemStackView View) const
{
	if (!View.IsValid()) return false;
	if (!View.Item->CanMutate() ) return false;
	if (!IsValid(ApplicationFilter)) return true;
	return ApplicationFilter->TryMatch(View);
}

bool UFaerieItemMutator::TryApply(const FFaerieItemStack& Stack, FSquirrelState* Squirrel)
{
	if (IsValid(ApplicationFilter) &&
		!ApplicationFilter->TryMatch(Stack))
	{
		return false;
	}

	return Apply(Stack, Squirrel);
}

void UFaerieItemMutator::GetRequiredAssets_Implementation(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}