// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemMutator.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemMutatorGroup.generated.h"

UENUM()
enum class EFaerieItemMutatorGroupPolicy : uint8
{
	ApplyFirst,

	// Any mutator that can apply will be applied.
	ApplyAny
};

/*
 * A group of mutators to apply together.
 */
USTRUCT()
struct FAERIEITEMGENERATOR_API FFaerieItemMutatorGroup final : public FFaerieItemMutator
{
	GENERATED_BODY()

#if WITH_EDITOR
	bool IsDataValid(FDataValidationContext& Context) const;
#endif

	virtual void GetRequiredAssets(TAdderRef<FSoftObjectPath> RequiredAssets) const override;
	virtual bool Apply(FFaerieItemInstance& Item, const FFaerieItemMutatorContext& Context) const override;

	[[nodiscard]] UE_REWRITE bool IsEmpty() const { return Children.IsEmpty(); }

protected:
	UPROPERTY(EditAnywhere, Category = "MutatorGroup", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FFaerieItemMutator>> Children;

	UPROPERTY(EditAnywhere, Category = "MutatorGroup")
	EFaerieItemMutatorGroupPolicy Policy = EFaerieItemMutatorGroupPolicy::ApplyAny;

	FAERIE_MUTATOR_HEADER(FFaerieItemMutatorGroup)
};