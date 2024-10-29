﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemToken.h"
#include "FaerieGuidToken.generated.h"

/**
 * A simple token adding a FGuid onto an item.
 * @WARNING THIS TOKEN HAS NOT BEEN TESTED, AND MIGHT NOT CREATE DESIRABLE BEHAVIOR
 * This is kinda a placeholder until I determine how/when the Guid should be (re)generated.
 */
UCLASS(DisplayName = "Token - Guid")
class FAERIEITEMDATA_API UFaerieGuidToken : public UFaerieItemToken
{
	GENERATED_BODY()

protected:
	virtual bool CompareWithImpl(const UFaerieItemToken* Other) const override
	{
		return CastChecked<ThisClass>(Other)->Guid == Guid;
	}

public:
	const FGuid& GetGuid() const { return Guid; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faerie|GuidToken")
	FGuid Guid;
};