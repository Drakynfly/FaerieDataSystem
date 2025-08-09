// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"
#include "FaerieItemTemplate.h"
#include "FaerieCraftingRunner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCraftingSubsystem)

void UFaerieItemCraftingSubsystem::Deinitialize()
{
	for (auto&& ActiveAction : ActiveActions)
	{
		ActiveAction->Cancel();
	}
	ActiveActions.Empty();

	Super::Deinitialize();
}

UFaerieCraftingRunner* UFaerieItemCraftingSubsystem::SubmitCraftingRequest(TInstancedStruct<FFaerieCraftingRequestBase> Request)
{
	if (!Request.IsValid())
	{
		return nullptr;
	}

	UFaerieCraftingRunner* ActiveAction = NewObject<UFaerieCraftingRunner>(this);
	ActiveActions.Add(ActiveAction);
	ActiveAction->GetOnCompletedCallback().BindUObject(this, &ThisClass::OnActionCompleted);
	ActiveAction->Start(Request);
	return ActiveAction;
}

void UFaerieItemCraftingSubsystem::OnActionCompleted(UFaerieCraftingRunner* Runner, EGenerationActionResult /*Result*/)
{
	ActiveActions.Remove(Runner);
}

void UFaerieItemCraftingSubsystem::SubmitCraftingRequest_Impl(const FFaerieRecipeCraftRequest& Request, const bool Preview)
{
	// With all slots validated, execute the upgrade.
	SubmitCraftingRequest(TInstancedStruct<FFaerieRecipeCraftRequest>::Make(Request));
}

void UFaerieItemCraftingSubsystem::PreviewCraftingRequest(const FFaerieRecipeCraftRequest& Request)
{
	SubmitCraftingRequest_Impl(Request, true);
}