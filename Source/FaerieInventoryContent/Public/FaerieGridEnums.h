// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/EnumRange.h"
#include "FaerieGridEnums.generated.h"

/* Degrees to rotate a shape by */
UENUM(BlueprintType)
enum class ESpatialItemRotation : uint8
{
	None = 0,
	Ninety = 1,
	One_Eighty = 2,
	Two_Seventy = 3,
	MAX UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ESpatialItemRotation, ESpatialItemRotation::MAX)

FORCEINLINE ESpatialItemRotation GetNextRotation(const ESpatialItemRotation CurrentRotation)
{
	switch (CurrentRotation)
	{
	case ESpatialItemRotation::None:
		return ESpatialItemRotation::Ninety;
	case ESpatialItemRotation::Ninety:
		return ESpatialItemRotation::One_Eighty;
	case ESpatialItemRotation::One_Eighty:
		return ESpatialItemRotation::Two_Seventy;
	case ESpatialItemRotation::Two_Seventy:
		return ESpatialItemRotation::None;
	default:
		return ESpatialItemRotation::None;
	}
}

FORCEINLINE ESpatialItemRotation GetPreviousRotation(const ESpatialItemRotation CurrentRotation)
{
	switch (CurrentRotation)
	{
	case ESpatialItemRotation::None:
		return ESpatialItemRotation::Two_Seventy;
	case ESpatialItemRotation::Ninety:
		return ESpatialItemRotation::None;
	case ESpatialItemRotation::One_Eighty:
		return ESpatialItemRotation::Ninety;
	case ESpatialItemRotation::Two_Seventy:
		return ESpatialItemRotation::One_Eighty;
	default:
		return ESpatialItemRotation::None;
	}
}

UENUM(BlueprintType)
enum class EFaerieGridEventType : uint8
{
	ItemAdded,
	ItemChanged,
	ItemRemoved
};