// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieUnownedItemStack.h"
#include "Engine/TimerHandle.h"
#include "Misc/DateTime.h"
#include "StructUtils/StructView.h"
#include "ItemCraftingAction.generated.h"

struct FStreamableHandle;
class UFaerieItemCraftingRunner;
class USquirrel;

UENUM(BlueprintType)
enum class EGenerationActionResult : uint8
{
	Failed,
	Timeout,
	Cancelled,
	Succeeded
};

USTRUCT(BlueprintType)
struct FFaerieCraftingActionHandle
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 Key = 0;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieCraftingActionHandle& Other) const
	{
		return Key == Other.Key;
	}

	friend uint32 GetTypeHash(const FFaerieCraftingActionHandle& Value)
	{
		return GetTypeHash(Value.Key);
	}
};

// The results of a crafting action
USTRUCT(BlueprintType)
struct FFaerieCraftingActionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CraftingActionData")
	TArray<FFaerieUnownedItemStack> Stacks;
};

namespace Faerie::Generation
{
	struct FActionExecution
	{
		// The object that is running this execution.
		TNotNull<UFaerieItemCraftingRunner*> Runner;

		// The world context for used by the action runner.
		TNotNull<UObject*> WorldContextObject;

		// The squirrel provided for deterministic generation (optional).
		TWeakObjectPtr<USquirrel> Squirrel = nullptr;

		bool IsInGameWorld() const;
	};

	using FActionResult = TDelegate<void(EGenerationActionResult Success, const FFaerieCraftingActionData&)>;
}

USTRUCT(BlueprintType)
struct FFaerieCraftingActionBase
{
	GENERATED_BODY()

	friend UFaerieItemCraftingRunner;
	friend struct FFaeriePrivate_CapturedCraftingAction;

	virtual ~FFaerieCraftingActionBase() = default;

	// Virtual run function. This must be implemented per subtype. It must finish before the timer runs out.
	virtual void Run(const Faerie::Generation::FActionExecution& Execution) PURE_VIRTUAL(FFaerieCraftingActionBase::Run, )

protected:
	static void CompleteWithResult(TStructView<FFaerieCraftingActionBase> ThisAction, const Faerie::Generation::FActionExecution& Execution, EGenerationActionResult Result);

	template <typename T>
	static void Complete(const Faerie::Generation::FActionExecution& Execution, T* Action)
	{
		CompleteWithResult(TStructView<FFaerieCraftingActionBase>(*Action), Execution, EGenerationActionResult::Succeeded);
	}

	template <typename T>
	static void Fail(const Faerie::Generation::FActionExecution& Execution, T* Action)
	{
		CompleteWithResult(TStructView<FFaerieCraftingActionBase>(*Action), Execution, EGenerationActionResult::Failed);
	}

	template <typename T>
	static void Cancel(const Faerie::Generation::FActionExecution& Execution, T* Action)
	{
		CompleteWithResult(TStructView<FFaerieCraftingActionBase>(*Action), Execution, EGenerationActionResult::Cancelled);
	}

	// Storage for result data.
	UPROPERTY()
	FFaerieCraftingActionData ActionData;

	// Handle to an async stream of assets we need.
	TSharedPtr<FStreamableHandle> RunningStreamHandle;

	// The handle to ourself in the runner.
	FFaerieCraftingActionHandle Handle;

	// Timer to shut off this action if it runs for too long.
	FTimerHandle TimerHandle;

#if WITH_EDITORONLY_DATA
	// Timestamp to record how long this action takes to run.
	FDateTime TimeStarted;
#endif

	Faerie::Generation::FActionResult OnCompletedCallback;
};