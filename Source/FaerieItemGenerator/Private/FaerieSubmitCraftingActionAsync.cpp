// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieSubmitCraftingActionAsync.h"
#include "FaerieItemCraftingSubsystem.h"

UFaerieSubmitCraftingActionAsync* UFaerieSubmitCraftingActionAsync::SubmitCraftingActionAsync(UObject* WorldContextObj,
	const TInstancedStruct<FFaerieCraftingRequestBase> Request)
{
	UFaerieSubmitCraftingActionAsync* Action = NewObject<UFaerieSubmitCraftingActionAsync>();
	Action->WorldContext = WorldContextObj;
	Action->Request = Request;
	return Action;
}

void UFaerieSubmitCraftingActionAsync::Activate()
{
	if (!IsValid(WorldContext))
	{
		return;
	}

	UFaerieItemCraftingSubsystem* CraftingSubsystem = WorldContext->GetWorld()->GetSubsystem<UFaerieItemCraftingSubsystem>();

	UFaerieCraftingRunner* Runner = CraftingSubsystem->SubmitCraftingRequest(Request,
		UFaerieItemCraftingSubsystem::FRequestResult::CreateUObject(this, &ThisClass::HandleResult));
}

void UFaerieSubmitCraftingActionAsync::HandleResult(const EGenerationActionResult GenerationActionResult, const TArray<FFaerieItemStack>& FaerieItemStacks)
{
	GenerationActionCompleted.Broadcast(GenerationActionResult, FaerieItemStacks);
}