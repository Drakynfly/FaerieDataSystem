// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubmitCraftingActionAsync.h"
#include "FaerieItemCraftingSubsystem.h"
#include "ItemCraftingRunner.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieSubmitCraftingActionAsync)

UFaerieSubmitCraftingActionAsync* UFaerieSubmitCraftingActionAsync::SubmitCraftingActionAsync(UObject* WorldContextObj,
	const TInstancedStruct<FFaerieCraftingActionBase> Request)
{
	UFaerieSubmitCraftingActionAsync* Action = NewObject<UFaerieSubmitCraftingActionAsync>();
	Action->WorldContext = WorldContextObj;
	Action->Action = Request;
	return Action;
}

void UFaerieSubmitCraftingActionAsync::Activate()
{
	if (!IsValid(WorldContext))
	{
		return;
	}

	UFaerieItemCraftingSubsystem* CraftingSubsystem = WorldContext->GetWorld()->GetSubsystem<UFaerieItemCraftingSubsystem>();

	Handle = CraftingSubsystem->GetRunner()->SubmitCraftingAction(Action, Faerie::Generation::FActionResult::CreateUObject(this, &ThisClass::HandleResult));
}

void UFaerieSubmitCraftingActionAsync::HandleResult(const EGenerationActionResult GenerationActionResult, const FFaerieCraftingActionData& FaerieItemStacks)
{
	GenerationActionCompleted.Broadcast(GenerationActionResult, FaerieItemStacks);
}

void UFaerieSubmitCraftingActionAsync::Cancel()
{
	UFaerieItemCraftingSubsystem* CraftingSubsystem = WorldContext->GetWorld()->GetSubsystem<UFaerieItemCraftingSubsystem>();
	CraftingSubsystem->CancelCraftingAction(Handle);
}