// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemInstance.h"
#include "ValidParameter.h"
#include "Misc/NotNull.h"

struct FMassEntityManager;
class UFaerieItemContainerBase;

namespace Faerie::Container
{
	// Validate that an item is valid. Used after loading an item from disk/data.
	// #@Todo move to ItemData module...
	FAERIEINVENTORY_API [[nodiscard]] bool ValidateItemData(const TValid<const FFaerieItemInstance&> Instance);

	FAERIEINVENTORY_API [[nodiscard]] UFaerieItemContainerBase* GetItemOwner(const FMassEntityManager& EntityManager, TValid<const FFaerieItemInstance&> Instance);

	// Finds the owner of an item, and calls ReleaseOwnership. WARNING: This is a low-level function: Use only if you know why.
	FAERIEINVENTORY_API void ClearOwnership(FMassEntityManager& EntityManager, TValid<const FFaerieItemInstance&> Instance);

	// This function must be called to bind items to a new owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void ReleaseOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, TValid<const FFaerieItemInstance&> Instance);

	// This function must be called to unbind items from an owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void TakeOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, TValid<FFaerieItemInstance&> Instance);
}
