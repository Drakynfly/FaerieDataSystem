﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemMutator_Condition.generated.h"

/*
 * Wrapper mutator that checks an Item Filter.
 */
USTRUCT(DisplayName = "Condition - Template")
struct FFaerieItemMutator_TemplateCondition final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual bool Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const override;

protected:
	// The filter that selects valid entries that this mutator can apply to.
	UPROPERTY(EditAnywhere, Category = "Mutator")
	TObjectPtr<UFaerieItemTemplate> ItemTemplate = nullptr;

	// The mutator to run, if the template is matched is passed.
	UPROPERTY(EditAnywhere, Category = "Template", meta = (ExcludeBaseStruct))
	TInstancedStruct<FFaerieItemMutator> Child;
};