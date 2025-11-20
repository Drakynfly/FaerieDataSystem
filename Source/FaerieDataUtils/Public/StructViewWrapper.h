// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "StructUtils/StructView.h"

#include "StructViewWrapper.generated.h"

/**
 * A blueprint wrapper around FStructView
 * Literally just a stub for now, I only added this to make RunMutatorRequest not need to copy its input from the bench.
 */
USTRUCT(BlueprintType)
struct FAERIEDATAUTILS_API FFaerieStructViewWrapper
{
	GENERATED_BODY()

	FStructView View;
};