// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemFilterTypes.generated.h"

UENUM()
enum class EItemDataMutabilityStatus : uint8
{
	Unknown,
	KnownMutable,
	KnownImmutable,
};