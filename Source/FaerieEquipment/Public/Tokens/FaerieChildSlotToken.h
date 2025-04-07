// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlot.h"
#include "Tokens/FaerieItemStorageToken.h"
#include "FaerieChildSlotToken.generated.h"

/**
 *
 */
UCLASS(DisplayName = "Token - Add Child Slot")
class FAERIEEQUIPMENT_API UFaerieChildSlotToken : public UFaerieItemContainerToken
{
	GENERATED_BODY()

public:
	UFaerieChildSlotToken();

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual void InitializeNetObject(AActor* Actor) override;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ChildSlot")
	UFaerieEquipmentSlot* GetSlotContainer() const;

protected:
	void OnSlotItemChanged(UFaerieEquipmentSlot* FaerieEquipmentSlot);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "EquipmentSlot")
	FFaerieEquipmentSlotConfig Config;

	UPROPERTY(EditInstanceOnly, Instanced, NoClear, Category = "EquipmentSlot")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;
};