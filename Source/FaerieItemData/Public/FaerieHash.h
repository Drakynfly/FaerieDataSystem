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

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieHash& Other) const
	{
		return Hash == Other.Hash;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieHash& Value) { return GetTypeHash(Value.Hash); }
};