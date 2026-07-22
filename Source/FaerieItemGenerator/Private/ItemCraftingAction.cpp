// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemCraftingAction.h"
#include "ItemCraftingRunner.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemCraftingAction)

bool Faerie::Generation::FActionExecution::IsInGameWorld() const
{
	return WorldContextObject->GetWorld()->IsGameWorld();
}

void FFaerieCraftingActionBase::CompleteWithResult(const TStructView<FFaerieCraftingActionBase> ThisAction, const Faerie::Generation::FActionExecution& Execution, const EGenerationActionResult Result)
{
	Execution.Runner->FinishAction(ThisAction, Result);
}
