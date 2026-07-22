// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeConfig.h"
#include "FaerieItem.h"
#include "FaerieItemMutator.h"
#include "ItemCraftingAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeConfig)

void UFaerieItemUpgradeConfigBase::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array) {}

#if WITH_EDITOR

#define LOCTEXT_NAMESPACE "FaerieItemUpgradeConfig_IsDataValid"

EDataValidationResult UFaerieItemUpgradeConfig::IsDataValid(FDataValidationContext& Context) const
{
	if (!Mutators.IsDataValid(Context))
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

void UFaerieItemUpgradeConfig::GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& Array)
{
	Mutators.GetRequiredAssets(Array);
}

bool UFaerieItemUpgradeConfig::CanPayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const
{
	return true;
}

void UFaerieItemUpgradeConfig::PayCost(TNotNull<UObject*> WorldContext, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const
{
}

bool UFaerieItemUpgradeConfig::ApplyUpgrade(TNotNull<UObject*> WorldContext, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	FFaerieItemMutatorContext_UpgradeConfig Context;
	Context.WorldContextObject = WorldContext;
	Context.Squirrel = Squirrel;
	Context.Config = this;

	for (auto&& OperationStack : Stacks.Stacks)
	{
		if (OperationStack.Copies == 0)
		{
			return false;
		}

		// Apply the mutator, and fail if it doesn't apply, when RequireMutatorToRun is enabled.
		Faerie::ItemData::FMutableReference Item(OperationStack.Instance);
		if (!Mutators.Apply(Item, Context) && RequireMutatorToRun)
		{
			return false;
		}

		// Reassign item pointer in case the mutator swapped it with a new instance.
		OperationStack.Instance = Item.GetInstance();
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanApplyUpgrade(const TNotNull<UObject*> WorldContext, const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanApplyUpgrade)))
	{
		return BP_CanApplyUpgrade(WorldContext, Proxy);
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanPayCost(const TNotNull<UObject*> WorldContext,
														const FFaerieCraftingFilledSlots& FilledSlots,
														const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanPayCost)))
	{
		return BP_CanPayCost(WorldContext, FilledSlots, Proxy);
	}
	return true;
}

void UFaerieItemUpgradeConfig_BlueprintBase::PayCost(const TNotNull<UObject*> WorldContext,
													 const FFaerieCraftingFilledSlots& FilledSlots,
													 const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_PayCost)))
	{
		BP_PayCost(WorldContext, FilledSlots, Proxy);
	}
}

bool UFaerieItemUpgradeConfig_BlueprintBase::ApplyUpgrade(const TNotNull<UObject*> WorldContext,
														  FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	return BP_ApplyUpgrade(WorldContext, Stacks, Squirrel);
}
