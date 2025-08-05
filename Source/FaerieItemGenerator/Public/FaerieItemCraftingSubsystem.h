// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "CraftingRequests.h"
#include "GenerationAction.h"
#include "Containers/SpscQueue.h"

#include "FaerieItemCraftingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogItemGeneratorSubsystem, Log, All)

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemCraftingSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void BeginRunningAction(UCraftingActionBase* Action);

	// Adds an action pointer that was created internally to the queue.
	void EnqueueAction_Internal(UCraftingActionBase* NewAction);

	void OnActionCompleted(EGenerationActionResult Success);

	/**
	 * Enqueues an Action to be executed in a FIFO queue.
	 * This is the no argument overload, only call this with classes that do not require configuration.
	 */
	template <
		typename TGenerationAction
		UE_REQUIRES(TIsDerivedFrom<TGenerationAction, UCraftingActionBase>::Value)
	>
	TGenerationAction* EnqueueActionTyped()
	{
		static_assert(!sizeof(TGenerationAction::FActionArgs), TEXT("If this asserts EnqueueActionTyped must be passed an Arguments parameter"));
		TGenerationAction* NewAction = NewObject<TGenerationAction>(this);
		EnqueueAction_Internal(NewAction);
		return NewAction;
	}

	/**
	 * Enqueues an Action to be executed in a FIFO queue.
	 * An arguments struct must be provided to configure the action.
	 * Configuration may not be performed on the returned pointer safely, as this action might already be executing if
	 * nothing else was queued.
	 */
	template <
		typename TGenerationAction
		UE_REQUIRES(TIsDerivedFrom<TGenerationAction, UCraftingActionBase>::Value)
	>
	TGenerationAction* EnqueueActionTyped(typename TGenerationAction::FActionArgs& Arguments)
	{
		TGenerationAction* NewAction = NewObject<TGenerationAction>(this);
		// This line will not compile if the action does not need to be configured.
		NewAction->Configure(Arguments);
		EnqueueAction_Internal(NewAction);
		return NewAction;
	}

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemGeneration")
	void SubmitGenerationRequest(const FGenerationRequest& Request);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemGeneration")
	void SubmitUpgradeRequest(const FUpgradeRequest& Request);

private:
	void SubmitCraftingRequest_Impl(const FCraftingRequest& Request, bool Preview);

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Faerie|ItemGeneration")
	void SubmitCraftingRequest(const FCraftingRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemGeneration")
	void PreviewCraftingRequest(const FCraftingRequest& Request);

private:
	// The Action currently running.
	UPROPERTY(Transient)
	TObjectPtr<UCraftingActionBase> ActiveAction;

	// Actions waiting to run
	TSpscQueue<TStrongObjectPtr<UCraftingActionBase>> PendingActions;
};