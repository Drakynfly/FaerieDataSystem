// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieCraftRecipeAction.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemRecipe.h"
#include "FaerieItemSource.h"
#include "ItemInstancingContext_Crafting.h"
#include "Recipes/FaerieRecipeCraftConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftRecipeAction)

void FFaerieCraftRecipeAction::Run(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Fail(Runner);
	}

	const FFaerieItemCraftingSlots CraftingSlots = Config->GetCraftingSlots();
	if (!Faerie::Generation::ValidateFilledSlots(Slots, CraftingSlots))
	{
		return Fail(Runner);
	}

	UE_LOG(LogItemGeneration, Log, TEXT("Running RecipeCraft"));

	if (!ensure(IsValid(Config->Recipe)))
	{
		return;
	}

	FFaerieItemInstancingContext_Crafting Context;
	Context.Squirrel = Squirrel.Get();
	Context.InputEntryData = Slots;

	const TOptional<FFaerieItemStack> NewStack = Config->Recipe->GetItemSource()->CreateItemStack(&Context);
	if (!NewStack.IsSet())
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Item Instancing failed for Craft Item!"));
		return Fail(Runner);
	}

	ActionData.Stacks.Add(NewStack.GetValue());

	if (RunConsumeStep && Config->Recipe->Implements<UFaerieItemSlotInterface>())
	{
		const FFaerieItemCraftingSlots SlotsView = Config->Recipe->GetCraftingSlots();
		Faerie::Generation::ConsumeSlotCosts(Slots, SlotsView);
	}

	return Complete(Runner);
}