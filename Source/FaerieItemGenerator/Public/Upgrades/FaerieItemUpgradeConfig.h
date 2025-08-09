// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "CraftingActionConfig.h"
#include "FaerieItemSlotInterface.h"
#include "FaerieItemUpgradeConfig.generated.h"

/**
 *
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieItemUpgradeConfig : public UFaerieCraftingActionConfig, public IFaerieItemSlotInterface
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual FFaerieCraftingSlotsView GetCraftingSlots() const override;

	// Mutator object, created inline.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Refinement Config")
	TObjectPtr<class UFaerieItemMutator> Mutator;
};