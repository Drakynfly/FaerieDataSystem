// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieCraftingRunner.h"
#include "FaerieGenerationStructs.h"
#include "FaerieItemGenerationRequest.generated.h"

class UFaerieItemGenerationConfig;

USTRUCT()
struct FFaerieItemGenerationRequestStorage : public FFaerieCraftingActionData
{
	GENERATED_BODY()

	// Children items to generate.
	TArray<Faerie::FPendingItemGeneration> PendingGenerations;
};

// The client assembles these via UI and submits them to the server for validation when requesting an item generation.
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationRequest : public FFaerieCraftingRequestBase
{
	GENERATED_BODY()

	virtual void Run(UFaerieCraftingRunner* Runner) const override;

protected:
	void LoadCheck(TSharedPtr<FStreamableHandle> Handle, UFaerieCraftingRunner* Runner) const;
	void Generate(UFaerieCraftingRunner* Runner) const;
	void ResolveGeneration(FFaerieItemGenerationRequestStorage& Storage, const Faerie::FPendingItemGeneration& Generation, const FFaerieItemInstancingContext_Crafting& Context) const;

	// The client must fill this with drivers that can have network ID mapping. This is automatic for serialized objects.
	// Runtime generated drivers must be created server-side and replicated for this to work.
	UPROPERTY(BlueprintReadWrite, Category = "Generation Request")
	TArray<TObjectPtr<UFaerieItemGenerationConfig>> Drivers;

	// Use pool assets to generate lists of drops, rather that use them as a source of a single drop
	UPROPERTY(BlueprintReadWrite, Category = "Crafting Request")
	bool RecursivelyResolveTables = false;
};