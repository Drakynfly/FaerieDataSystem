// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieCraftRecipeAction.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemRecipe.h"
#include "FaerieItemSource.h"
#include "ItemInstancingContext_Crafting.h"
#include "Recipes/FaerieRecipeCraftConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftRecipeAction)

void FFaerieCraftRecipeAction::Run(UFaerieCraftingRunner* Runner) const
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Runner->Fail();
	}

	FFaerieCraftingActionData RequestData;

	if (const FFaerieCraftingSlotsView SlotsView = Faerie::Generation::GetCraftingSlots(Config);
		SlotsView.IsValid())
	{
		if (!Faerie::Generation::ValidateFilledSlots(Slots, SlotsView.Get()))
		{
			return Runner->Fail();
		}
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
		return Runner->Fail();
	}

	RequestData.ProcessStacks.Add(NewStack.GetValue());

	if (RunConsumeStep && Config->Recipe->Implements<UFaerieItemSlotInterface>())
	{
		if (const FFaerieCraftingSlotsView SlotsView = Faerie::Generation::GetCraftingSlots(Config->Recipe);
			SlotsView.IsValid())
		{
			Faerie::Generation::ConsumeSlotCosts(Slots, SlotsView.Get());
		}
	}

	Runner->RequestStorage.InitializeAs<FFaerieCraftingActionData>(RequestData);

	Runner->Complete();
}