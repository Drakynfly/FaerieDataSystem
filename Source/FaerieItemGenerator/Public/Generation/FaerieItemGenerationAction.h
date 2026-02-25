// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemCraftingAction.h"
#include "FaerieGenerationStructs.h"
#include "FaerieItemGenerationAction.generated.h"

class UFaerieItemGenerationConfig;

//
USTRUCT()
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationActionBase : public FFaerieCraftingActionBase
{
	GENERATED_BODY()
};

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationActionSingle : public FFaerieItemGenerationActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

protected:
	void LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, TNotNull<UFaerieItemCraftingRunner*> Runner);
	void Generate(TNotNull<UFaerieItemCraftingRunner*> Runner);

	UPROPERTY(BlueprintReadWrite, Category = "Generation Action Single")
	FFaerieTableDrop Source;
};

//
USTRUCT(BlueprintType)
struct FAERIEITEMGENERATOR_API FFaerieItemGenerationAction : public FFaerieItemGenerationActionBase
{
	GENERATED_BODY()

	virtual void Run(TNotNull<UFaerieItemCraftingRunner*> Runner) override;

protected:
	void LoadDrivers(TNotNull<UFaerieItemCraftingRunner*> Runner);
	void LoadCheck(const TSharedPtr<FStreamableHandle>& LoadHandle, TNotNull<UFaerieItemCraftingRunner*> Runner, int32 CheckFromNum);
	void Generate(TNotNull<UFaerieItemCraftingRunner*> Runner);

	UPROPERTY(BlueprintReadWrite, Category = "Generation Action")
	TArray<TSoftObjectPtr<UFaerieItemGenerationConfig>> Drivers;

	// Use pool assets to generate lists of drops, rather that use them as a source of a single drop
	UPROPERTY(BlueprintReadWrite, Category = "Generation Action")
	bool RecursivelyResolveTables = false;

private:
	// Children items to generate.
	TArray<Faerie::Generation::FPendingTableDrop> PendingGenerations;
};