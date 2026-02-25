// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemCraftingAction.h"
#include "ItemCraftingRunner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemCraftingAction)

void FFaerieCraftingActionBase::Cancel(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	Runner->FinishAction(Handle, EGenerationActionResult::Cancelled);
}

void FFaerieCraftingActionBase::Complete(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	Runner->FinishAction(Handle, EGenerationActionResult::Succeeded);
}

void FFaerieCraftingActionBase::Fail(const TNotNull<UFaerieItemCraftingRunner*> Runner)
{
	Runner->FinishAction(Handle, EGenerationActionResult::Failed);
}