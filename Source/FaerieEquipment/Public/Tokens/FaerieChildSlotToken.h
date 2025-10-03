// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlot.h"
#include "Tokens/FaerieItemStorageToken.h"
#include "FaerieChildSlotToken.generated.h"

/**
 *
 */
UCLASS(DisplayName = "Token - Add Child Slot")
class FAERIEEQUIPMENT_API UFaerieChildSlotToken : public UFaerieItemContainerToken, public IFaerieContainerExtensionInterface
{
	GENERATED_BODY()

public:
	UFaerieChildSlotToken();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	//~ UNetSupportedObject
	virtual void InitializeNetObject(AActor* Actor) override;
	virtual void DeinitializeNetObject(AActor* Actor) override;
	//~ UNetSupportedObject

	//~ IFaerieContainerExtensionInterface
	virtual UItemContainerExtensionGroup* GetExtensionGroup() const override final { return Extensions; }
	//~ IFaerieContainerExtensionInterface

	UFUNCTION(BlueprintCallable, Category = "Faerie|ChildSlot")
	UFaerieEquipmentSlot* GetSlotContainer() const;

protected:
	void OnSlotItemChanged(UFaerieItemStackContainer* Slot, FFaerieInventoryTag Event);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Replicated, Category = "EquipmentSlot")
	FFaerieEquipmentSlotConfig Config;

	UPROPERTY(EditInstanceOnly, Instanced, Replicated, NoClear, Category = "EquipmentSlot")
	TObjectPtr<UItemContainerExtensionGroup> Extensions;
};