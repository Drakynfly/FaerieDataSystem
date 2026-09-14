// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Actions/FaerieClientActionBase.h"
#include "StackContainerActions.generated.h"

// @todo move these to FaerieInventory module

class UFaerieItemStackContainer;

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveFromStackContainer final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool View(Faerie::ItemData::FScopeProxy& Proxy) const override;
	virtual bool CanMove(Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;
	virtual bool Release(FFaerieUnownedItemStack& OutStack) const override;
	virtual bool Possess(Faerie::TValid<const FFaerieUnownedItemStack&> InStack) const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveFromStackContainer")
	TObjectPtr<UFaerieItemStackContainer> Stack = nullptr;
};

USTRUCT(BlueprintType)
struct FFaerieClientAction_MoveToStackContainer final : public FFaerieClientAction_MoveHandlerBase
{
	GENERATED_BODY()

	virtual bool IsValid(TNotNull<const UFaerieInventoryClient*> Client) const override;
	virtual bool View(Faerie::ItemData::FScopeProxy& Proxy) const override;
	virtual bool CanMove(Faerie::TValid<const FFaerieItemProxy&> Proxy) const override;
	virtual bool Release(FFaerieUnownedItemStack& OutStack) const override;
	virtual bool Possess(Faerie::TValid<const FFaerieUnownedItemStack&> InStack) const override;
	virtual bool IsSwap() const override;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToStackContainer")
	TObjectPtr<UFaerieItemStackContainer> Stack = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "MoveToStackContainer")
	bool CanSwapContent = true;
};