// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
#include "UObject/Object.h"
#include "FaerieItemMutator.generated.h"

class USquirrel;
class UFaerieItemTemplate;

/**
 * Base class for mutation behavior. This is essentially a 'command' class.
 * GetRequiredAssets is optional to implement.
 * Apply must be implemented.
 */
USTRUCT()
struct FFaerieItemMutator
{
	GENERATED_BODY()

	virtual ~FFaerieItemMutator() = default;

	// Any soft assets required to be loaded when Apply is called should be registered here.
	virtual void GetRequiredAssets(TArray<TSoftObjectPtr<UObject>>& RequiredAssets) const {}

	//
	virtual bool Apply(FFaerieItemStack& Stack, USquirrel* Squirrel) const PURE_VIRTUAL(FFaerieItemMutator::Apply, return false; )
};