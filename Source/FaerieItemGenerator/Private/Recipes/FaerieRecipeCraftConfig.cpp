// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieRecipeCraftConfig.h"
#include "FaerieItemRecipe.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieRecipeCraftConfig)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieRecipeCraftConfig_IsDataValid"

EDataValidationResult UFaerieRecipeCraftConfig::IsDataValid(FDataValidationContext& Context) const
{
	if (!Recipe)
	{
		Context.AddError(LOCTEXT("InvalidSourceAsset", "Source Asset is invalid!"));
	}

	if (Context.GetNumErrors())
	{
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context);
}

#undef LOCTEXT_NAMESPACE

#endif

FFaerieItemCraftingSlots UFaerieRecipeCraftConfig::GetCraftingSlots() const
{
	return Recipe->GetCraftingSlots();
}