// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRunner.h"
#include "StructViewWrapper.h"
#include "RunMutatorRequest.generated.h"

struct FFaerieItemMutator;

/*
 * A simple crafting request to run a mutator. Pass this to UFaerieItemCraftingSubsystem::SubmitCraftingRequest to run
 * asynchronous mutators.
*/
USTRUCT(BlueprintType)
struct FFaerieRunMutatorRequest : public FFaerieCraftingRequestBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

private:
	void Execute(UFaerieCraftingRunner* Runner) const;

public:
	// The stacks to run the mutator on.
	UPROPERTY(BlueprintReadWrite, Category = "Upgrade Request")
	TArray<FFaerieItemStack> ItemStacks;

	// Mutator struct, created inline.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Config")
	FFaerieStructViewWrapper Mutator;
};
