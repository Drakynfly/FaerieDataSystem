// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Mutators/ItemMutator_Condition.h"
#include "FaerieItemProxy.h"
#include "FaerieItemSource.h"
#include "FaerieItemTemplate.h"
#include "MassEntityManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemMutator_Condition)

FAERIE_MUTATOR_IMPL(FFaerieItemMutator_TemplateCondition)

void FFaerieItemMutator_TemplateCondition::GetRequiredAssets(const TAdderRef<FSoftObjectPath> RequiredAssets) const
{
	Mutators.GetRequiredAssets(RequiredAssets);
}

bool FFaerieItemMutator_TemplateCondition::Apply(Faerie::ItemData::FItemDataBuilder& Builder, const FFaerieItemMutatorContext& Context) const
{
	if (IsValid(ItemTemplate))
	{
		const TSharedPtr<FMassEntityManager> EntityManagerShared = Context.EntityManager ? Context.EntityManager->AsShared().ToSharedPtr() : nullptr;
		if (!ItemTemplate->Matches(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManagerShared, &Builder)))
		{
			// Template failed, cannot apply.
			return false;
		}
	}

	return Mutators.Apply(Builder, Context);
}
