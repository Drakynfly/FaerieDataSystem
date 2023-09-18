﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "InventoryDataStructs.h"

#include "FaerieStackLimiterToken.generated.h"


UCLASS()
class FAERIEINVENTORY_API UFaerieStackLimiterToken : public UFaerieItemToken
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MaxStackSizeToken")
	static int32 GetItemStackLimit(const UFaerieItem* Item);

	UFUNCTION(BlueprintCallable, Category = "MaxStackSizeToken")
	int32 GetStackLimit() const;

protected:
	// Max stack size
	UPROPERTY(EditAnywhere, meta = (ClampMin = 0))
	int32 MaxStackSize;
};