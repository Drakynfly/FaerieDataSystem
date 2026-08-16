// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieCraftRecipeAction.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemRecipe.h"
#include "FaerieItemSource.h"
#include "ItemCraftingRunner.h"
#include "ItemInstancingContext_Crafting.h"
#include "Recipes/FaerieRecipeCraftConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftRecipeAction)

void FFaerieCraftRecipeAction::Run(const Faerie::Generation::FActionExecution& Execution)
{
	if (!IsValid(Config))
	{
		return Fail(Execution, this, INVTEXT("Invalid Recipe config"));
	}

	if (NewInstancesOuter == nullptr)
	{
		NewInstancesOuter = GetTransientPackageAsObject();
	}

	if (const FFaerieItemCraftingSlots* SlotsPtr = Config->Recipe->GetCraftingSlots())
	{
		if (!Faerie::Generation::ValidateFilledSlots<true>(Execution.EntityManager, Slots, *SlotsPtr))
		{
			return Fail(Execution, this, INVTEXT("Validation of input slots failed"));
		}
	}


	UE_LOG(LogItemGeneration, Log, TEXT("Running RecipeCraft"));

	if (!ensure(IsValid(Config->Recipe)))
	{
		return;
	}

	FFaerieItemInstancingContext_Crafting Context;
	Context.EntityManager = Execution.EntityManager;
	Context.Squirrel = Execution.Squirrel.Get();
	Context.InputEntryData = Slots;

	const Faerie::ItemData::FGetInstanceResult Result = Config->Recipe->GetItemSource()->CreateItemStack(Context);
	if (!Result.IsValid())
	{
		return Fail(Execution, this, INVTEXT("Item Instancing failed for Craft Item!"));
	}

	ActionData.Stacks.Add(Result.WithInitialization());

	if (RunConsumeStep)
	{
		check(Execution.EntityManager);
		if (const FFaerieItemCraftingSlots* SlotsPtr = Config->Recipe->GetCraftingSlots())
		{
			Faerie::Generation::ConsumeSlotCosts(*Execution.EntityManager, Slots, *SlotsPtr);
		}
	}

	return Complete(Execution, this, FText::GetEmpty());
}