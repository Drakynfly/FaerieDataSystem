// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemCraftingSubsystem.h"
#include "ItemCraftingRunner.h"
#include "Squirrel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCraftingSubsystem)

using namespace Faerie;

void UFaerieItemCraftingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Squirrel = NewObject<USquirrel>(this);
	Runner = NewObject<UFaerieItemCraftingRunner>(this);
	Runner->SetSquirrel(Squirrel);
}

void UFaerieItemCraftingSubsystem::Deinitialize()
{
	Runner->CancelAllActions();
	Runner = nullptr;

	Super::Deinitialize();
}

FFaerieCraftingActionHandle UFaerieItemCraftingSubsystem::SubmitCraftingRequest(
	const TInstancedStruct<FFaerieCraftingActionBase> Request, const FGenerationActionOnCompleteBinding& Callback)
{
	return Runner->SubmitCraftingRequest(Request, Callback);
}

void UFaerieItemCraftingSubsystem::CancelCraftingAction(const FFaerieCraftingActionHandle Handle)
{
	return Runner->CancelCraftingAction(Handle);
}
