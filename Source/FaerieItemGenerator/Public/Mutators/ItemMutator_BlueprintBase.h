// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "ItemMutator_BlueprintBase.generated.h"

struct FFaerieItemProxy;

/*
 * Wrapper to apply a Blueprint Mutator Class.
 */
USTRUCT()
struct FFaerieItemMutator_Blueprint final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(FFaerieItemInstance& Item, const FFaerieItemMutatorContext& Context) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blueprint")
	TSubclassOf<class UFaerieItemMutator_BlueprintBase> Blueprint;

	FAERIE_MUTATOR_HEADER(FFaerieItemMutator_Blueprint)
};

/*
 * Base class for defining mutators in Blueprint.
 * @Note for now this class does not support the ability to swap the item instance out for a new one.
 */
UCLASS(Abstract, Blueprintable, const)
class FAERIEITEMGENERATOR_API UFaerieItemMutator_BlueprintBase : public UObject
{
	GENERATED_BODY()

	friend FFaerieItemMutator_Blueprint;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Mutator")
	bool Apply(const FFaerieItemProxy& Proxy, USquirrel* Squirrel) const;

	// Any soft assets required to be loaded when Apply is called should be registered here.
	UFUNCTION(BlueprintNativeEvent, Category = "Mutator")
	void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const;
};