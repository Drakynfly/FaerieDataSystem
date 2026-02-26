// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeConfig.h"
#include "FaerieItemMutator.h"
#include "ItemCraftingAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeConfig)

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemUpgradeConfig_IsDataValid"

void UFaerieItemUpgradeConfigBase::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array) {}

bool UFaerieItemUpgradeConfigBase::ConsumeSlotCosts(const FFaerieCraftingFilledSlots& FilledSlots,
	const FFaerieItemCraftingSlots& CraftingSlots)
{
	return Faerie::Generation::ConsumeSlotCosts(FilledSlots, CraftingSlots);
}

EDataValidationResult UFaerieItemUpgradeConfig::IsDataValid(FDataValidationContext& Context) const
{
	if (!Mutator.IsValid())
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

FFaerieItemCraftingSlots UFaerieItemUpgradeConfig::GetCraftingSlots() const
{
	return GetCraftingSlots(FFaerieItemStackView());
}

bool UFaerieItemUpgradeConfig::CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const
{
	const FFaerieItemCraftingSlots CraftingSlots = GetCraftingSlots(View);
	if (!Faerie::Generation::ValidateFilledSlots(FilledSlots, CraftingSlots))
	{
		return false;
	}
	return true;
}

void UFaerieItemUpgradeConfig::PayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const
{
	const FFaerieItemCraftingSlots CraftingSlots = GetCraftingSlots(View);
	Faerie::Generation::ConsumeSlotCosts(FilledSlots, CraftingSlots);
}

FFaerieItemCraftingSlots UFaerieItemUpgradeConfig::GetCraftingSlots(const FFaerieItemStackView View) const
{
	return FFaerieItemCraftingSlots();
}

bool UFaerieItemUpgradeConfig::ApplyUpgrade(FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	FFaerieItemMutatorContext_UpgradeConfig Context;
	Context.Squirrel = Squirrel;
	Context.Config = this;

	for (auto&& OperationStack : Stacks.Stacks)
	{
		if (OperationStack.Copies == 0)
		{
			return false;
		}

		// Apply the mutator, and fail if it doesn't apply, when RequireMutatorToRun is enabled.
		if (!Mutator.Get().Apply(OperationStack, &Context) && RequireMutatorToRun)
		{
			return false;
		}
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanApplyUpgrade(const FFaerieItemStackView View) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanApplyUpgrade)))
	{
		return BP_CanApplyUpgrade(View);
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanPayCost(const FFaerieCraftingFilledSlots& FilledSlots,
														const FFaerieItemStackView View) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanPayCost)))
	{
		return BP_CanPayCost(FilledSlots, View);
	}
	return true;
}

void UFaerieItemUpgradeConfig_BlueprintBase::PayCost(const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemStackView View) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_PayCost)))
	{
		BP_PayCost(FilledSlots, View);
	}
}

bool UFaerieItemUpgradeConfig_BlueprintBase::ApplyUpgrade(FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	return BP_ApplyUpgrade(Stacks, Squirrel);
}
