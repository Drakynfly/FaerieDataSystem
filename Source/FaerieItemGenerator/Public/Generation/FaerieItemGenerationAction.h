// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "FaerieGenerationStructs.h"
#include "FaerieItemGenerationAction.generated.h"

class UFaerieItemGenerationConfig;

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationActionSingle : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(const Faerie::Generation::FActionExecution& Execution) override;

protected:
	void LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, const Faerie::Generation::FActionExecution& Execution);
	void Generate(const Faerie::Generation::FActionExecution& Execution);

	UPROPERTY(BlueprintReadWrite, Category = "Generation Action Single")
	FFaerieTableDrop Source;

private:
	// Storage for loaded assets to keep alive while running this action.
	UPROPERTY()
	TArray<TObjectPtr<const UObject>> LoadedAssets;
};

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationAction : public FFaerieCraftingActionBase
{
	GENERATED_BODY()

	virtual void Run(const Faerie::Generation::FActionExecution& Execution) override;

protected:
	void LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, const Faerie::Generation::FActionExecution& Execution, int32 CheckFromNum);
	void Generate(const Faerie::Generation::FActionExecution& Execution);

	UPROPERTY(BlueprintReadWrite, Category = "Generation Action")
	TArray<TSoftObjectPtr<UFaerieItemGenerationConfig>> Drivers;

	// Use pool assets to generate lists of drops, rather that use them as a source of a single drop
	UPROPERTY(BlueprintReadWrite, Category = "Generation Action")
	bool RecursivelyResolveTables = false;

private:
	// @Todo this could use an inline allocator...
	TArray<Faerie::Generation::FPendingTableDrop> PendingDrops;

	// Storage for loaded assets to keep alive while running this action.
	UPROPERTY()
	TArray<TObjectPtr<const UObject>> LoadedAssets;
};