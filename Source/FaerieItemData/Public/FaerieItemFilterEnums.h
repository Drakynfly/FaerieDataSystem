// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemFilterEnums.generated.h"

UENUM()
enum class EFaerieItemDataMutabilityStatus : uint8
{
	Unknown,
	KnownMutable,
	KnownImmutable,
};

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