// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "Mass/ExternalSubsystemTraits.h"
#include "FaerieMeshStructs.h"
#include "FaerieMeshFragment.generated.h"

USTRUCT(BlueprintType)
struct FFaerieMeshFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeshFragment")
	FFaerieMeshContainer Container;

#if WITH_EDITOR
	EDataValidationResult IsDataValid(FDataValidationContext& Context) const;
#endif
};

template<>
struct TMassFragmentTraits<FFaerieMeshFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};