// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemFilterEnums.generated.h"

UENUM()
enum class EItemDataMutabilityStatus : uint8
{
	Unknown,
	KnownMutable,
	KnownImmutable,
};