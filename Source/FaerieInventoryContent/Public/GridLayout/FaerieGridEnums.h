// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/EnumRange.h"
#include "FaerieGridEnums.generated.h"

/* Degrees to rotate a shape by */
UENUM(BlueprintType)
enum class EFaerieSpatialItemRotation : uint8
{
	None = 0,
	Ninety = 1,
	One_Eighty = 2,
	Two_Seventy = 3,
	MAX UMETA(Hidden)
};

namespace Faerie::Spatial
{
	[[nodiscard]] UE_REWRITE constexpr EFaerieSpatialItemRotation AddRotations(EFaerieSpatialItemRotation A, EFaerieSpatialItemRotation B)
	{
		using T = __underlying_type(EFaerieSpatialItemRotation);
		const T Sum = static_cast<T>(A) + static_cast<T>(B);
		return static_cast<EFaerieSpatialItemRotation>(Sum % static_cast<T>(EFaerieSpatialItemRotation::MAX));
	}

	[[nodiscard]] UE_REWRITE constexpr EFaerieSpatialItemRotation GetNextRotation(const EFaerieSpatialItemRotation CurrentRotation)
    {
    	switch (CurrentRotation)
    	{
    	case EFaerieSpatialItemRotation::None:
    		return EFaerieSpatialItemRotation::Ninety;
    	case EFaerieSpatialItemRotation::Ninety:
    		return EFaerieSpatialItemRotation::One_Eighty;
    	case EFaerieSpatialItemRotation::One_Eighty:
    		return EFaerieSpatialItemRotation::Two_Seventy;
    	case EFaerieSpatialItemRotation::Two_Seventy:
    		return EFaerieSpatialItemRotation::None;
    	default:
    		return EFaerieSpatialItemRotation::None;
    	}
    }

    [[nodiscard]] UE_REWRITE constexpr EFaerieSpatialItemRotation GetPreviousRotation(const EFaerieSpatialItemRotation CurrentRotation)
    {
    	switch (CurrentRotation)
    	{
    	case EFaerieSpatialItemRotation::None:
    		return EFaerieSpatialItemRotation::Two_Seventy;
    	case EFaerieSpatialItemRotation::Ninety:
    		return EFaerieSpatialItemRotation::None;
    	case EFaerieSpatialItemRotation::One_Eighty:
    		return EFaerieSpatialItemRotation::Ninety;
    	case EFaerieSpatialItemRotation::Two_Seventy:
    		return EFaerieSpatialItemRotation::One_Eighty;
    	default:
    		return EFaerieSpatialItemRotation::None;
    	}
    }
}


UENUM(BlueprintType)
enum class EFaerieGridEventType : uint8
{
	ItemAdded,
	ItemChanged,
	ItemRemoved
};