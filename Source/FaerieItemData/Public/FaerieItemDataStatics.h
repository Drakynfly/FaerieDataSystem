// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

class UFaerieItem;

namespace Faerie
{
	// Validate that an item is valid. Used after loading an item from disk/data.
	[[nodiscard]] FAERIEITEMDATA_API bool ValidateLoadedItem(const UFaerieItem* Item);
}
