// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
#include "Actions/FaerieClientActionBase.h"
#include "FaerieEquipmentClientActions.generated.h"

class UFaerieEquipmentSlot;

// Request that the server set the item in a slot.
USTRUCT(BlueprintType)
struct FFaerieClientAction_SetItemInSlot final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "RequestSetItemInSlot")
	TObjectPtr<UFaerieEquipmentSlot> Slot = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "RequestSetItemInSlot")
	FFaerieItemStack Stack;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveFromSlot final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(const UFaerieInventoryClient* Client) const override;
	virtual bool View(FFaerieItemStackView& View) const override;
	virtual bool CanMove(const FFaerieItemStackView& View) const override;
	virtual bool Release(FFaerieItemStack& Stack) const override;
	virtual bool Possess(const FFaerieItemStack& Stack) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromSlot")
	TObjectPtr<UFaerieEquipmentSlot> Slot = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveToSlot final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(const UFaerieInventoryClient* Client) const override;
	virtual bool View(FFaerieItemStackView& View) const override;
	virtual bool CanMove(const FFaerieItemStackView& View) const override;
	virtual bool Release(FFaerieItemStack& Stack) const override;
	virtual bool Possess(const FFaerieItemStack& Stack) const override;
	virtual bool IsSwap() const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToSlot")
	TObjectPtr<UFaerieEquipmentSlot> Slot = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToSlot")
	bool CanSwapSlots = true;
};