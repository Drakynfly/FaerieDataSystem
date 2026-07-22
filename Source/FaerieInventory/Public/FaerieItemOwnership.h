// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "Misc/NotNull.h"

class UFaerieItemContainerBase;

namespace Faerie::Container
{
	// Validate that an item is valid. Used after loading an item from disk/data.
	// #@Todo move to ItemData module...
	FAERIEINVENTORY_API [[nodiscard]] bool ValidateItemData(const ItemData::FReference& Reference);

	FAERIEINVENTORY_API [[nodiscard]] UFaerieItemContainerBase* GetItemOwner(const ItemData::FRequireEntityManager& EntityManager, const ItemData::FMutableReference& Instance);

	// This function must be called to bind items to a new owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void ReleaseOwnership(const ItemData::FRequireEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance);

	// Finds the owner of an item, and calls ReleaseOwnership. WARNING: This is a low-level function: Use only if you know why.
	FAERIEINVENTORY_API void ClearOwnership(const ItemData::FRequireEntityManager& EntityManager, const ItemData::FMutableReference& Instance);

	// This function must be called to unbind items from an owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void TakeOwnership(const ItemData::FRequireEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, const ItemData::FMutableReference& Instance);
}
