// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieMassTypeTraits.h"
#include "FaerieItemOwningContainer.generated.h"

class UFaerieItemContainerBase;

USTRUCT()
struct FAERIEINVENTORY_API FFaerieMassItemOwningContainer : public FFaerieMassConstSharedFragment
{
	GENERATED_BODY()

	FFaerieMassItemOwningContainer() = default;
	FFaerieMassItemOwningContainer(UFaerieItemContainerBase* Owner)
	  : Owner(Owner) {}

	UPROPERTY()
	TObjectPtr<UFaerieItemContainerBase> Owner;

____FAERIE_FRAGMENT_DECL(FFaerieMassItemOwningContainer)
};

template <>
struct Faerie::ItemData::TMassTypeTraits<FFaerieMassItemOwningContainer> : TMassTypeTraitsBase<FFaerieMassItemOwningContainer>
{
	enum
	{
		// Runtime pointer to container doesn't serialize, it gets restored by TakeOwnership, or via replication
		Serialized = false
	};
};