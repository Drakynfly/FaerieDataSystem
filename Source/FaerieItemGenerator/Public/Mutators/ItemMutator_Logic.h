// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemMutator_Logic.generated.h"

/*
 * Wrapper mutator that applies only the first mutator to succeed.
 */
USTRUCT(DisplayName = "Apply First")
struct FFaerieItemMutator_ApplyFirst final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Template", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemMutator>> Children;
};

/*
 * Wrapper mutator that applies any of its children that it can.
 */
USTRUCT(DisplayName = "Apply Any")
struct FFaerieItemMutator_ApplyAny final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Template", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemMutator>> Children;
};

/*
 * Wrapper mutator that either applies all of its children, or none, if one cannot apply.
 */
USTRUCT(DisplayName = "Apply All")
struct FFaerieItemMutator_ApplyAll final : public FFaerieItemMutator
{
	GENERATED_BODY()

	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const override;
	virtual bool Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Template", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemMutator>> Children;
};