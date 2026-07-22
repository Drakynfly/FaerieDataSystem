// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_Condition.h"
#include "FaerieItemDataView.h"
#include "FaerieItemTemplate.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_Condition)

FAERIE_MUTATOR_IMPL(FFaerieItemMutator_TemplateCondition)

void FFaerieItemMutator_TemplateCondition::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const
{
	Mutators.GetRequiredAssets(RequiredAssets);
}

bool FFaerieItemMutator_TemplateCondition::Apply(Faerie::ItemData::FMutableReference& Item, const FFaerieItemMutatorContext& Context) const
{
	if (IsValid(ItemTemplate))
	{
		const FFaerieItemDataView View(Item, 1, nullptr);
		if (!ItemTemplate->TryMatch(Context.WorldContextObject, View))
		{
			// Template failed, cannot apply.
			return false;
		}
	}

	return Mutators.Apply(Item, Context);
}
