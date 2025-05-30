﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Actions/FaerieInventoryClient.h"
#include "InventoryDataStructs.h"
#include "GridExtensionClientActions.generated.h"

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveToGrid final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(const UFaerieInventoryClient* Client) const override;
	virtual bool CanMove(const FFaerieItemStackView& View) const override;
	virtual bool Possess(const FFaerieItemStack& Stack) const override;
	virtual bool View(FFaerieItemStackView& View) const override;
	virtual bool Release(FFaerieItemStack& Stack) const override;
	virtual bool IsSwap() const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToGrid")
	TObjectPtr<UFaerieItemStorage> Storage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToGrid")
	FIntPoint Position = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToGrid")
	bool CanSwapSlots = true;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveItemOnGrid final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveItemOnGrid")
	FInventoryKeyHandle Handle;

	UPROPERTY(BlueprintReadWrite, Category = "MoveItemOnGrid")
	FIntPoint DragEnd = FIntPoint::ZeroValue;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_RotateGridEntry final : public FFaerieClientActionBase
{
	GENERATED_BODY()

	virtual bool Server_Execute(const UFaerieInventoryClient* Client) const override;

	UPROPERTY(BlueprintReadWrite, Category = "RotateGridEntry")
	FInventoryKeyHandle Handle;

	// @todo should also pass a 90 degree / 180 degree parameter...
};