// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeConfig.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeConfig)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemUpgradeConfig_IsDataValid"

EDataValidationResult UFaerieItemUpgradeConfig::IsDataValid(FDataValidationContext& Context) const
{
	if (!Mutator)
	{
		Context.AddError(LOCTEXT("MutatorNotValid", "Mutators is invalid."));
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

#endif

FFaerieCraftingSlotsView UFaerieItemUpgradeConfig::GetCraftingSlots() const
{
	return FFaerieCraftingSlotsView();
	//return Faerie::Crafting::GetCraftingSlots(Mutator);
}