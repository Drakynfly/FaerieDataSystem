// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Recipes/FaerieCraftRecipeAction.h"
#include "FaerieItemGenerationLog.h"
#include "FaerieItemRecipe.h"
#include "FaerieItemSource.h"
#include "ItemCraftingRunner.h"
#include "ItemInstancingContext_Crafting.h"
#include "Recipes/FaerieRecipeCraftConfig.h"
#include "EntityManagerHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCraftRecipeAction)

void FFaerieCraftRecipeAction::Run(const Faerie::Generation::FActionExecution& Execution)
{
	if (!IsValid(Config))
	{
		UE_LOG(LogItemGeneration, Warning, TEXT("%hs: Config is invalid!"), __FUNCTION__);
		return Fail(Execution, this);
	}

	if (NewInstancesOuter == nullptr)
	{
		NewInstancesOuter = GetTransientPackageAsObject();
	}

	if (const FFaerieItemCraftingSlots* SlotsPtr = Config->Recipe->GetCraftingSlots())
	{
		if (!Faerie::Generation::ValidateFilledSlots(Execution.WorldContextObject, Slots, *SlotsPtr))
		{
			return Fail(Execution, this);
		}
	}


	UE_LOG(LogItemGeneration, Log, TEXT("Running RecipeCraft"));

	if (!ensure(IsValid(Config->Recipe)))
	{
		return;
	}

	FFaerieItemInstancingContext_Crafting Context;
	Context.ItemInstanceOuter = NewInstancesOuter;
	Context.Squirrel = Execution.Squirrel.Get();
	Context.InputEntryData = Slots;

	const Faerie::ItemData::FGetInstanceResult Result = Config->Recipe->GetItemSource()->CreateItemStack(Context);
	if (!Result.IsValid())
	{
		UE_LOG(LogItemGeneration, Error, TEXT("Item Instancing failed for Craft Item!"));
		return Fail(Execution, this);
	}

	ActionData.Stacks.Add(Result.WithInitialization());

	if (RunConsumeStep)
	{
		Faerie::ItemData::FOptionalEntityManager EntityManager(Execution.WorldContextObject);
		if (const FFaerieItemCraftingSlots* SlotsPtr = Config->Recipe->GetCraftingSlots())
		{
			Faerie::Generation::ConsumeSlotCosts(EntityManager, Slots, *SlotsPtr);
		}
	}

	return Complete(Execution, this);
}