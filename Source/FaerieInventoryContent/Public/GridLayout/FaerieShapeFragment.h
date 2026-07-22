// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "SpatialTypes.h"
#include "FaerieShapeFragment.generated.h"

USTRUCT(BlueprintType)
struct FFaerieShapeFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieShapeFragment() = default;
	FFaerieShapeFragment(const FFaerieGridShape& Shape)
	  : Shape(Shape) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ShowOnlyInnerProperties))
	FFaerieGridShape Shape;
};

// @Todo make shape stored by pointer or something so this can be removed
template<>
struct TMassFragmentTraits<FFaerieShapeFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};