// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Actions/FaerieClientActionBase.h"
#include "FaerieEquipmentClientActions.generated.h"

class UFaerieEquipmentSlot;

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveFromSlot final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool View(FFaerieItemDataView& View) const override;
	virtual bool CanMove(const FFaerieItemDataView& View) const override;
	virtual bool Release(FFaerieUnownedItemStack& Stack) const override;
	virtual bool Possess(const FFaerieUnownedItemStack& Stack) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromSlot")
	TObjectPtr<UFaerieEquipmentSlot> Slot = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveToSlot final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool View(FFaerieItemDataView& View) const override;
	virtual bool CanMove(const FFaerieItemDataView& View) const override;
	virtual bool Release(FFaerieUnownedItemStack& Stack) const override;
	virtual bool Possess(const FFaerieUnownedItemStack& Stack) const override;
	virtual bool IsSwap() const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToSlot")
	TObjectPtr<UFaerieEquipmentSlot> Slot = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToSlot")
	bool CanSwapSlots = true;
};