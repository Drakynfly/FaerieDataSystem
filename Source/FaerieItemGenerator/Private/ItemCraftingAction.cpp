// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemCraftingAction.h"
#include "ItemCraftingRunner.h"
#include "MassEntityManager.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemCraftingAction)

bool Faerie::Generation::FActionExecution::IsInGameWorld() const
{
	if (!EntityManager || !EntityManager->GetWorld())
	{
		return false;
	}
	return EntityManager->GetWorld()->IsGameWorld();
}

void FFaerieCraftingActionBase::CompleteWithResult(const TStructView<FFaerieCraftingActionBase> ThisAction, const Faerie::Generation::FActionExecution& Execution, const EGenerationActionResult Result, const FText& Message)
{
	Execution.Runner->FinishAction(ThisAction, Result, Message);
}
