// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "ItemSlotHandle.h"

#include "GenerationAction.generated.h"

struct FFaerieItemSlotHandle;
struct FFaerieItemStack;
class IFaerieItemDataProxy;
class USquirrel;

DECLARE_LOG_CATEGORY_EXTERN(LogGenerationAction, Log, All)

UENUM(BlueprintType)
enum class EGenerationActionResult : uint8
{
	Failed,
	Timeout,
	Succeeded
};

using FNativeGenerationActionCompletedCallback = TDelegate<void(EGenerationActionResult)>;
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGenerationActionOnCompleteBinding, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGenerationActionCompleted, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);

USTRUCT(BlueprintType)
struct FCraftingActionSparseClassStruct
{
	GENERATED_BODY()

	FCraftingActionSparseClassStruct() = default;

	// The maximum duration an action can run, in seconds, before timing out.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Configuration")
	float DefaultTimeoutTime = 10.f;
};

class IFaerieItemSlotInterface;
class UFaerieItemCraftingSubsystem;

/**
 *
 */
UCLASS(Abstract, ClassGroup = "Faerie", Within = FaerieItemCraftingSubsystem, SparseClassDataTypes = CraftingActionSparseClassStruct)
class FAERIEITEMGENERATOR_API UCraftingActionBase : public UObject
{
	GENERATED_BODY()

public:
	struct FActionArgs
	{
		TWeakObjectPtr<USquirrel> Squirrel;
		FGenerationActionOnCompleteBinding Callback;
	};

	UCraftingActionBase();

	virtual UWorld* GetWorld() const override;

protected:
	// Optional overload, to preload assets before Run is executed.
	virtual TArray<FSoftObjectPath> GetAssetsToLoad() const { return {}; }

	// Internal virtual run function. This must be implemented per subclass. It must finish before the timer has ran out.
	virtual void Run() PURE_VIRTUAL(UGenerationActionBase::Run, )

	bool IsRunning() const;

private:
	FTimerManager& GetTimerManager() const;

	void OnTimeout();

	// This MUST be called during the actions execution or this action will be timed-out.
	void Finish(EGenerationActionResult Result);

protected:
	void Configure(FActionArgs& Args);

public:
	FNativeGenerationActionCompletedCallback::RegistrationType& GetOnCompletedCallback() { return OnCompletedCallback; }

protected:
	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Complete();

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Fail();

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Start();

protected:
	UPROPERTY()
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGenerationActionCompleted OnCompleted;

	UPROPERTY()
	TArray<FFaerieItemStack> ProcessStacks;

private:
	UPROPERTY()
	FTimerHandle TimerHandle;

#if WITH_EDITORONLY_DATA
	// Running state of the action. All errors related to this should be discovered and resolved in the editor.
	UPROPERTY()
	bool IsRunning_EditorOnlyTracker = false;

	// Timestamp to record how long this action takes to run.
	UPROPERTY()
	FDateTime TimeStarted;
#endif

	FNativeGenerationActionCompletedCallback OnCompletedCallback;
};

UCLASS(Abstract)
class FAERIEITEMGENERATOR_API UCraftingActionWithSlots : public UCraftingActionBase
{
	GENERATED_BODY()

protected:
	// Remove items and durability from the entries in Slots used to fund this action.
	void ConsumeSlotCosts(const IFaerieItemSlotInterface* Interface);

public:
	struct FActionArgs : Super::FActionArgs
	{
		bool RunConsumeStep = false;
		TMap<FFaerieItemSlotHandle, FFaerieItemProxy> FilledSlots;
	};

	void Configure(FActionArgs& Args);

protected:
	virtual void Run() override;

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|GenerationAction")
	FFaerieItemProxy GetProxyForSlot(const FFaerieItemSlotHandle& Filter, bool ExpectPresence = false) const;

protected:
	// Should ConsumeSlotCosts be called during Run. This is disabled to preview output before commiting to the action.
	UPROPERTY()
	bool RunConsumeStep = false;

	// Crafting slots, and the inventory key being used to provide data to them.
	UPROPERTY()
	TMap<FFaerieItemSlotHandle, FFaerieItemProxy> FilledSlots;
};