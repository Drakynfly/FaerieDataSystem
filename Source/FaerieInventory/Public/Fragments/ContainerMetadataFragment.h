// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "GameplayTagContainer.h"
#include "ContainerMetadataFragment.generated.h"

// Enables items to allow/disallow certain actions from being performed on it while stored in a container.
USTRUCT()
struct FAERIEINVENTORY_API FFaerieContainerMetadataFragment : public FFaerieMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ContainerMetadata", meta = (Categories = "Fae.Inventory"))
	FGameplayTagContainer AllowedActions;

	UPROPERTY(EditAnywhere, Category = "ContainerMetadata", meta = (Categories = "Fae.Inventory"))
	FGameplayTagContainer DisallowedActions;

____FAERIE_FRAGMENT_DECL(FFaerieContainerMetadataFragment)
};

template<>
struct TMassFragmentTraits<FFaerieContainerMetadataFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};
