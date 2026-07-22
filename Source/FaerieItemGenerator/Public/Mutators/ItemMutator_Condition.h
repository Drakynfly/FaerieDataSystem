// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "ItemMutatorGroup.h"

#include "StructUtils/InstancedStruct.h"
#include "ItemMutator_Condition.generated.h"

class UFaerieItemTemplate;

/*
 * Wrapper mutator that checks an Item Filter.
 */
USTRUCT()
struct FFaerieItemMutator_TemplateCondition final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(Faerie::ItemData::FMutableReference& Item, const FFaerieItemMutatorContext& Context) const override;

protected:
	// The filter that selects valid entries that this mutator can apply to.
	UPROPERTY(EditAnywhere, Category = "MutatorTemplateCondition")
	TObjectPtr<UFaerieItemTemplate> ItemTemplate = nullptr;

	UPROPERTY(EditAnywhere, Category = "MutatorTemplateCondition", meta = (ExcludeBaseStruct))
	FFaerieItemMutatorGroup Mutators;

	FAERIE_MUTATOR_HEADER(FFaerieItemMutator_TemplateCondition)
};