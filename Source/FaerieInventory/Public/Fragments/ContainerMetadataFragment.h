// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "GameplayTagContainer.h"
#include "ContainerMetadataFragment.generated.h"

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
