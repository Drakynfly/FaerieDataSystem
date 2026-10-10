// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Mass/EntityElementTypes.h"
#include "FaerieItemOwningContainer.generated.h"

class UFaerieItemContainerBase;

USTRUCT()
struct FAERIEINVENTORY_API FFaerieMassItemOwningContainer : public FMassConstSharedFragment
{
	GENERATED_BODY()

	FFaerieMassItemOwningContainer() = default;
	FFaerieMassItemOwningContainer(UFaerieItemContainerBase* Owner)
	  : Owner(Owner) {}

	UPROPERTY()
	TObjectPtr<UFaerieItemContainerBase> Owner;
};