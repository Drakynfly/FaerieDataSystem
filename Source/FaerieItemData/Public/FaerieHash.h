// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Templates/TypeHash.h"

#include "FaerieHash.generated.h"

/*
 * The result of a faerie hashing operation.
 * This is just a uint32 wrapped for type-safety and Blueprint access.
 */
USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/FaerieItemData.FaerieItemDataStructsLibrary.BreakFaerieHash"))
struct FAERIEITEMDATA_API FFaerieHash
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "FaerieHash")
	uint32 Hash = 0;

	friend bool operator==(const FFaerieHash& Lhs, const FFaerieHash& Rhs)
	{
		return Lhs.Hash == Rhs.Hash;
	}

	friend bool operator!=(const FFaerieHash& Lhs, const FFaerieHash& Rhs) { return !(Lhs == Rhs); }

	FORCEINLINE friend uint32 GetTypeHash(const FFaerieHash& Arg) { return GetTypeHash(Arg.Hash); }
};