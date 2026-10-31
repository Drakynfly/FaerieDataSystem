// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Upgrades/FaerieItemUpgradeConfig.h"
#include "FaerieMassFragmentAccessors.h"
#include "FaerieItemMutator.h"
#include "ItemCraftingAction.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemUpgradeConfig)

void UFaerieItemUpgradeConfigBase::GetRequiredAssets(const TAdderRef<FSoftObjectPath> Array) {}

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

void UFaerieItemUpgradeConfig::GetRequiredAssets(const TAdderRef<FSoftObjectPath> Array)
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

bool UFaerieItemUpgradeConfig::ApplyUpgrade(FMassEntityManager* EntityManager, Faerie::Generation::FActionStorage& Storage, USquirrel* Squirrel) const
{
	FFaerieItemMutatorContext_UpgradeConfig Context;
	Context.EntityManager = EntityManager;
	Context.Squirrel = Squirrel;
	Context.Config = this;

	for (Faerie::ItemData::FItemDataBuilder& Builder : Storage.Builders)
	{
		if (Builder.GetCopies() == 0)
		{
			return false;
		}

		// Apply the mutator, and fail if it doesn't apply, when RequireMutatorToRun is enabled.
		if (!Mutators.Apply(Builder, Context) && RequireMutatorToRun)
		{
			return false;
		}
	}
	return true;
}

bool UFaerieItemUpgradeConfig_BlueprintBase::CanApplyUpgrade(const FFaerieItemProxy& Proxy) const
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
														  Faerie::Generation::FActionStorage& Storage, USquirrel* Squirrel) const
{
	bool Success = true;
	for (Faerie::ItemData::FItemDataBuilder& Builder : Storage.Builders)
	{
		const TSharedPtr<FMassEntityManager> EntityManagerShared = EntityManager ? EntityManager->AsShared().ToSharedPtr() : nullptr;
		if (!BP_ApplyUpgrade(FFaerieItemProxy(FFaerieItemProxy::ESingleFrame, EntityManagerShared, &Builder), Squirrel))
		{
			Success = false;
		}
	}
	return Success;
}
