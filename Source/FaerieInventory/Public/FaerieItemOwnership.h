// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

#define FAE_API FAERIEINVENTORY_API

struct FMassEntityHandle;
struct FFaerieItemInstance;
struct FMassEntityManager;
class UFaerieItemContainerBase;

namespace Faerie::Container
{
	// Validate that an item is valid. Used after loading an item from disk/data.
	// #@Todo move to ItemData module...
	[[nodiscard]] FAE_API bool ValidateItemData(const FFaerieItemInstance& Instance);

	[[nodiscard]] FAE_API FMassEntityHandle GetContainerOwner(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	[[nodiscard]] FAE_API UFaerieItemContainerBase* GetItemOwner(const FMassEntityManager& EntityManager, FMassEntityHandle Item);

	// Finds the owner of an item, and calls ReleaseOwnership. WARNING: This is a low-level function: Use only if you know why.
	FAE_API void ClearOwnership(FMassEntityManager& EntityManager, FMassEntityHandle Item);

	// This function must be called to bind items to a new owner. Nested items are recursed over, so only call the root.
	FAE_API void ReleaseOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, FMassEntityHandle Item);

	// This function must be called to unbind items from an owner. Nested items are recursed over, so only call the root.
	FAE_API void TakeOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Instance);
}

#undef FAE_API