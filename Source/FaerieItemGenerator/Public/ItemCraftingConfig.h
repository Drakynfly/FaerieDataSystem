// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CraftingActionConfig.h"
#include "FaerieItemSlotInterface.h"
#include "ItemCraftingConfig.generated.h"

class UFaerieItemRecipe;

DECLARE_LOG_CATEGORY_EXTERN(LogItemCraftingConfig, Log, All);

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UItemCraftingConfig : public UCraftingActionConfig, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

	UItemCraftingConfig() = default;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	virtual FFaerieCraftingSlotsView GetCraftingSlots() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting Config")
	TObjectPtr<UFaerieItemRecipe> Recipe;
};

/**
 * Allows any blueprint to create an Item Crafting Config.
 */
USTRUCT(BlueprintType)
struct FOnTheFlyItemCraftingConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, NoClear, Category = "On The Fly Item Crafter")
	TObjectPtr<UItemCraftingConfig> Config = nullptr;
};