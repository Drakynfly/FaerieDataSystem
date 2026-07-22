// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "HAL/Platform.h"

namespace Faerie::ItemData
{
	// Utility to insert -1 into code to mark "All Copies of a Stack"
	inline constexpr int32 EntireStack = -1;

	// Utility to insert -1 into code to mark "A Stack doesn't have a Limit"
	inline constexpr int32 UnlimitedStack = -1;
}
