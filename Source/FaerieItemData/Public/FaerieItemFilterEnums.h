// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemFilterEnums.generated.h"

UENUM()
enum class EFaerieItemDataMutabilityStatus : uint8
{
	Unknown,
	KnownMutable,
	KnownImmutable,
	Conflict
};

namespace Faerie
{
	inline EFaerieItemDataMutabilityStatus CombineStatuses(const EFaerieItemDataMutabilityStatus A, const EFaerieItemDataMutabilityStatus B)
	{
		if (A == EFaerieItemDataMutabilityStatus::Conflict || B == EFaerieItemDataMutabilityStatus::Conflict)
		{
			return EFaerieItemDataMutabilityStatus::Conflict;
		}

		if (A == EFaerieItemDataMutabilityStatus::Unknown) return B;
		if (B == EFaerieItemDataMutabilityStatus::Unknown) return A;

		if (A == B)
		{
			return A;
		}

		return EFaerieItemDataMutabilityStatus::Conflict;
	}
}

UENUM()
enum class EFaerieCopiesCompareOperator : uint8
{
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Greater			UMETA(DisplayName = ">"),
	GreaterOrEqual	UMETA(DisplayName = ">="),
	Equal			UMETA(DisplayName = "=="),
	NotEqual		UMETA(DisplayName = "!="),
};

UENUM()
enum class EFaerieStackCompareOperator : uint8
{
	Less			UMETA(DisplayName = "<"),
	LessOrEqual		UMETA(DisplayName = "<="),
	Greater			UMETA(DisplayName = ">"),
	GreaterOrEqual	UMETA(DisplayName = ">="),
	Equal			UMETA(DisplayName = "=="),
	NotEqual		UMETA(DisplayName = "!="),
	HasLimit		UMETA(DisplayName = "Limited"),
	HasNoLimit		UMETA(DisplayName = "Unlimited")
};