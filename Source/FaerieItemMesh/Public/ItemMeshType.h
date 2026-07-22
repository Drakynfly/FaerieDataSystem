// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemMeshType.generated.h"

UENUM(BlueprintType)
enum class EItemMeshType : uint8
{
	None UMETA(hidden),
	Static,
	Skeletal
};