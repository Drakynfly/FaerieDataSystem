// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "UObject/Object.h"
#include "ItemMutator_BlueprintBase.generated.h"

/*
 * Wrapper to apply a Blueprint Mutator Class.
 */
USTRUCT(DisplayName = "Blueprint Mutator")
struct FFaerieItemMutator_Blueprint final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(FFaerieItemStack& Stack, FFaerieItemMutatorContext* Context) const override;

protected:
	UPROPERTY()
	TObjectPtr<class UFaerieItemMutator_BlueprintBase> Blueprint;
};

/*
 * Base class for defining mutators in Blueprint.
 */
UCLASS(Abstract, const)
class FAERIEITEMGENERATOR_API UFaerieItemMutator_BlueprintBase : public UObject
{
	GENERATED_BODY()

	friend FFaerieItemMutator_Blueprint;

protected:
	void NativeGetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const;
	bool NativeApply(FFaerieItemStack& Stack, USquirrel* Squirrel) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Mutator")
	bool Apply(UPARAM(ref) FFaerieItemStack& Stack, USquirrel* Squirrel) const;

	// Any soft assets required to be loaded when Apply is called should be registered here.
	UFUNCTION(BlueprintNativeEvent, Category = "Mutator")
	void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const;
};