// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
#include "FaerieItemStackView.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "FaerieItemOwnerInterface.generated.h"

class UFaerieItemToken;

UINTERFACE(NotBlueprintable, MinimalAPI)
class UFaerieItemOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * This interface is used to interact with an object that owns UFaerieItems.
 * This interface is *not* required in order to own an item.
 */
class FAERIEITEMDATA_API IFaerieItemOwnerInterface
{
	GENERATED_BODY()

public:
	// Call this function to request release of ownership of a UFaerieItem stack. If the release is granted, the returned
	// stack will contain a valid pointer and the number of released copies. The returned stack size is guaranteed to be
	// only *up to* the amount requested. Partial release behavior is up to the implementing class.
	[[nodiscard]] virtual FFaerieItemStack Release(FFaerieItemStackView Stack) PURE_VIRTUAL(IFaerieItemOwnerInterface::Release, return FFaerieItemStack(); )

	// Call this function to grant ownership of a UFaerieItem stack. Returns true if ownership was accepted.
	// It is implied, and is the responsibility of the implementing class, to either accept ownership of the whole stack,
	// or none. Partial possession is not allowed.
	[[nodiscard]] virtual bool Possess(FFaerieItemStack Stack) PURE_VIRTUAL(IFaerieItemOwnerInterface::Possess, return false; )

//protected:
	// Note: this should be protected, not public, but i don't have a workaround for this yet.
	// This is an optional function to override to add logic when an item mutates while owned by this.
	virtual void OnItemMutated(const UFaerieItem* Item, const UFaerieItemToken* Token, FGameplayTag EditTag) {}
};