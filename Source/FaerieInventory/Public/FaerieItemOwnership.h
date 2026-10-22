// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Misc/NotNull.h"

#define FAE_API FAERIEINVENTORY_API

struct FFaerieItemProxy;
struct FMassEntityHandle;
struct FFaerieItemInstance;
struct FMassEntityManager;
class UFaerieItemContainerBase;

namespace Faerie::Container
{
	// RAII type to block TakeOwnership when intending to handle them later, typically during deserialization of nested
	// items. Obviously, this is a loaded shotgun that will blow your leg off if used incorrectly. Only use if you are
	// *sure* you are calling TakeOwnership again later, on the same frame as you've blocked it. This only exists to
	// stop TakeOwnership from freaking out that the Owner doesn't yet have an Actor outer, which is normal during import,
	// but prevents replication from being registered.
	struct FAE_API FBlockTakeOwnershipScope
	{
		UE_NONCOPYABLE(FBlockTakeOwnershipScope)
		FBlockTakeOwnershipScope();
		~FBlockTakeOwnershipScope();
	};

	// Validate that an item is valid. Used after loading an item from disk/data.
	// #@Todo move to ItemData module...
	[[nodiscard]] FAE_API bool ValidateItemData(const FFaerieItemInstance& Instance);

	[[nodiscard]] FAE_API FMassEntityHandle GetContainerOwner(const FMassEntityManager& EntityManager, TNotNull<const UFaerieItemContainerBase*> Container);

	[[nodiscard]] FAE_API UFaerieItemContainerBase* GetItemOwner(const FMassEntityManager& EntityManager, FMassEntityHandle Item);
	[[nodiscard]] FAE_API UFaerieItemContainerBase* GetItemOwner(const FFaerieItemProxy& Proxy);

	// Finds the owner of an item, and calls ReleaseOwnership. WARNING: This is a low-level function: Use only if you know why.
	FAE_API void ClearOwnership(FMassEntityManager& EntityManager, FMassEntityHandle Item);

	// This function must be called to bind items to a new owner. Nested items are recursed over, so only call the root.
	FAE_API void ReleaseOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, FMassEntityHandle Item);

	// This function must be called to unbind items from an owner. Nested items are recursed over, so only call the root.
	FAE_API void TakeOwnership(FMassEntityManager& EntityManager, TNotNull<UFaerieItemContainerBase*> Owner, const FMassEntityHandle Instance);
}

#undef FAE_API