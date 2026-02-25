// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
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
	TArray<FFaerieItemStack> Stacks;
};

namespace Faerie::Generation
{
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
	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) PURE_VIRTUAL(FFaerieCraftingActionBase::Run, )

	FFaerieCraftingActionHandle GetHandle() const { return Handle; }

protected:
	void Cancel(TNotNull<UFaerieItemCraftingRunner*> Runner);
	void Complete(TNotNull<UFaerieItemCraftingRunner*> Runner);
	void Fail(TNotNull<UFaerieItemCraftingRunner*> Runner);

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Action")
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY()
	FFaerieCraftingActionData ActionData;

	TSharedPtr<FStreamableHandle> RunningStreamHandle;

	FFaerieCraftingActionHandle Handle;

	FTimerHandle TimerHandle;

#if WITH_EDITORONLY_DATA
	// Timestamp to record how long this action takes to run.
	UPROPERTY()
	FDateTime TimeStarted;
#endif

	Faerie::Generation::FActionResult OnCompletedCallback;
};