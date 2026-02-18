// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Engine/TimerHandle.h"
#include "StructUtils/InstancedStruct.h"

#include "FaerieCraftingRunner.generated.h"

struct FFaerieItemStack;
struct FStreamableHandle;
class FTimerManager;
class UFaerieCraftingRunner;
class UFaerieItemCraftingSubsystem;
class USquirrel;

UENUM(BlueprintType)
enum class EGenerationActionResult : uint8
{
	Failed,
	Timeout,
	Cancelled,
	Succeeded
};

namespace Faerie::Generation
{
	using FActionComplete = TDelegate<void(UFaerieCraftingRunner*, EGenerationActionResult)>;
}

USTRUCT(BlueprintType)
struct FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual ~FFaerieCraftingActionBase() = default;

	// Virtual run function. This must be implemented per subtype. It must finish before the timer runs out.
	virtual void Run(UFaerieCraftingRunner* Runner) const
		PURE_VIRTUAL(FFaerieCraftingRequestBase::Run, )

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TWeakObjectPtr<USquirrel> Squirrel;
};

// The results of a crafting action
USTRUCT(BlueprintType)
struct FFaerieCraftingActionData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFaerieItemStack> ProcessStacks;
};

/**
 *
 */
UCLASS(ClassGroup = "Faerie", Within = FaerieItemCraftingSubsystem)
class FAERIEITEMGENERATOR_API UFaerieCraftingRunner final : public UObject
{
	GENERATED_BODY()

	friend UFaerieItemCraftingSubsystem;

private:
	// This MUST be called during the actions execution or this action will be timed-out.
	void Finish(EGenerationActionResult Result);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Complete();

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Fail();

public:
	// Custom storage for data required during runtime
	UPROPERTY()
	TInstancedStruct<FFaerieCraftingActionData> RequestStorage;

	TSharedPtr<FStreamableHandle> RunningStreamHandle;

private:
	// Keep the Action alive while we are running.
	UPROPERTY()
	TInstancedStruct<FFaerieCraftingActionBase> RunningAction;

	UPROPERTY()
	FTimerHandle TimerHandle;

#if WITH_EDITORONLY_DATA
	// Timestamp to record how long this action takes to run.
	UPROPERTY()
	FDateTime TimeStarted;
#endif

	Faerie::Generation::FActionComplete OnCompletedCallback;
};