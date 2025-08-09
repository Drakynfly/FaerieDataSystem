// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "ItemSlotHandle.h"
#include "Engine/TimerHandle.h"
#include "StructUtils/InstancedStruct.h"

#include "FaerieCraftingRunner.generated.h"

struct FInstancedStruct;
struct FStreamableHandle;
struct FFaerieItemStack;
class IFaerieItemDataProxy;
class UFaerieCraftingRunner;
class USquirrel;

UENUM(BlueprintType)
enum class EGenerationActionResult : uint8
{
	Failed,
	Timeout,
	Cancelled,
	Succeeded
};

namespace Faerie
{
	using FGenerationActionComplete = TDelegate<void(UFaerieCraftingRunner*, EGenerationActionResult)>;
}
DECLARE_DYNAMIC_DELEGATE_TwoParams(FGenerationActionOnCompleteBinding, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGenerationActionCompleted, EGenerationActionResult, Result, const TArray<FFaerieItemStack>&, Items);

USTRUCT(BlueprintType)
struct FFaerieCraftingRequestBase
{
	GENERATED_BODY()

	virtual ~FFaerieCraftingRequestBase() = default;

	virtual bool Configure(UFaerieCraftingRunner* Runner) const { return true; }
	virtual bool LoadAssets(UFaerieCraftingRunner* Runner) const { return false; }

	// Virtual run function. This must be implemented per subtype. It must finish before the timer has ran out.
	virtual void Run(UFaerieCraftingRunner* Runner) const
		PURE_VIRTUAL(FFaerieCraftingRequestBase::Run, )

	// The squirrel provided for deterministic generation (optional).
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	FGenerationActionOnCompleteBinding OnComplete;
};


USTRUCT(BlueprintType)
struct FCraftingActionSparseClassStruct
{
	GENERATED_BODY()

	FCraftingActionSparseClassStruct() = default;

	// The maximum duration an action can run, in seconds, before timing out.
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Configuration", meta = (Units = s))
	float DefaultTimeoutTime = 30.f;
};

class IFaerieItemSlotInterface;
class UFaerieItemCraftingSubsystem;

/**
 *
 */
UCLASS(ClassGroup = "Faerie", Within = FaerieItemCraftingSubsystem, SparseClassDataTypes = CraftingActionSparseClassStruct)
class FAERIEITEMGENERATOR_API UFaerieCraftingRunner final : public UObject
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;

private:
	FTimerManager& GetTimerManager() const;

	void OnTimeout();

	// This MUST be called during the actions execution or this action will be timed-out.
	void Finish(EGenerationActionResult Result);

protected:
	void Run();

public:
	Faerie::FGenerationActionComplete::RegistrationType& GetOnCompletedCallback() { return OnCompletedCallback; }

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Start(TInstancedStruct<FFaerieCraftingRequestBase>& Request);

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Cancel();

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Complete();

	UFUNCTION(BlueprintCallable, Category = "Faerie|CraftingAction")
	void Fail();

protected:
	UPROPERTY()
	TWeakObjectPtr<USquirrel> Squirrel;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGenerationActionCompleted OnCompleted;

public:
	// @todo should not be public
	UPROPERTY()
	TArray<FFaerieItemStack> ProcessStacks;

	// Custom storage for data required during runtime
	UPROPERTY()
	FInstancedStruct RequestStorage;

	TSharedPtr<FStreamableHandle> RunningStreamHandle;

private:
	UPROPERTY()
	TInstancedStruct<FFaerieCraftingRequestBase> RunningRequest;

	UPROPERTY()
	FTimerHandle TimerHandle;


#if WITH_EDITORONLY_DATA
	// Timestamp to record how long this action takes to run.
	UPROPERTY()
	FDateTime TimeStarted;
#endif

	Faerie::FGenerationActionComplete OnCompletedCallback;
};

// @todo move elsewhere
USTRUCT()
struct FFaerieCraftingActionSlots
{
	GENERATED_BODY()

	// Crafting slots, and the proxy being used to provide data to them.
	UPROPERTY()
	TMap<FFaerieItemSlotHandle, FFaerieItemProxy> FilledSlots;
};

namespace Faerie
{
	// Remove items and durability from the entries in Slots used to fund this action.
	void ConsumeSlotCosts(const TMap<FFaerieItemSlotHandle, FFaerieItemProxy>& Slots, const IFaerieItemSlotInterface* Interface);
}