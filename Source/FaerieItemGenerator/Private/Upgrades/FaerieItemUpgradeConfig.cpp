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

bool UFaerieItemUpgradeConfig::CanPayCost(const FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const
{
	return true;
}

void UFaerieItemUpgradeConfig::PayCost(FMassEntityManager* EntityManager, const FFaerieCraftingFilledSlots& FilledSlots, const FFaerieItemProxy& Proxy) const
{
}

bool UFaerieItemUpgradeConfig::ApplyUpgrade(FMassEntityManager* EntityManager, FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	FFaerieItemMutatorContext_UpgradeConfig Context;
	Context.EntityManager = EntityManager;
	Context.Squirrel = Squirrel;
	Context.Config = this;

	for (auto&& OperationStack : Stacks.Stacks)
	{
		if (OperationStack.Copies == 0)
		{
			return false;
		}

		// Apply the mutator, and fail if it doesn't apply, when RequireMutatorToRun is enabled.
		if (!Mutators.Apply(OperationStack.Instance, Context) && RequireMutatorToRun)
		{
			return false;
		}
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanApplyUpgrade(const FMassEntityManager* EntitManager, const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanApplyUpgrade)))
	{
		return BP_CanApplyUpgrade(Proxy);
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanPayCost(const FMassEntityManager* EntityManager,
														const FFaerieCraftingFilledSlots& FilledSlots,
														const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_CanPayCost)))
	{
		return BP_CanPayCost(FilledSlots, Proxy);
	}
	return true;
}

void UFaerieItemUpgradeConfig_BlueprintBase::PayCost(FMassEntityManager* EntityManager,
													 const FFaerieCraftingFilledSlots& FilledSlots,
													 const FFaerieItemProxy& Proxy) const
{
	if (GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(ThisClass, BP_PayCost)))
	{
		BP_PayCost(FilledSlots, Proxy);
	}
}

bool UFaerieItemUpgradeConfig_BlueprintBase::ApplyUpgrade(FMassEntityManager* EntityManager,
														  FFaerieCraftingActionData& Stacks, USquirrel* Squirrel) const
{
	bool Success = true;
	for (auto&& Stack : Stacks.Stacks)
	{
		Faerie::ItemData::FScopeProxy Proxy(Stack.Instance, Stack.Copies, nullptr);
		if (!BP_ApplyUpgrade(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, &Proxy), Squirrel))
		{
			Success = false;
		}
	}
	return Success;
}
