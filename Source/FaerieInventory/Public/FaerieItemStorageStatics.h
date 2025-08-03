// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"

class UObject;
class UFaerieItem;

namespace Faerie
{
	// Validate that an item is valid. Used after loading an item from disk/data.
	FAERIEINVENTORY_API [[nodiscard]] bool ValidateItemData(const UFaerieItem* Item);

	// This function must be called to bind items to a new owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void ReleaseOwnership(UObject* Owner, const UFaerieItem* Item);

	// Finds the owner of an item, and calls ReleaseOwnership. WARNING: This is a low-level function: Use only if you know why.
	FAERIEINVENTORY_API void ClearOwnership(const UFaerieItem* Item);

	// This function must be called to unbind items from an owner. Nested items are recursed over, so only call the root.
	FAERIEINVENTORY_API void TakeOwnership(UObject* Owner, const UFaerieItem* Item);
}
