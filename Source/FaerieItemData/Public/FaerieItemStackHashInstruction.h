// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "FaerieItemStackHashInstruction.generated.h"

/**
 * Base type for modular hashing routines.
 */
USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemDataHashInstruction
{
	GENERATED_BODY()

	virtual ~FFaerieItemDataHashInstruction() = default;
	virtual uint32 Hash(const FMassEntityManager* EntityManager, Faerie::TValid<const FFaerieItemProxy&> View) const PURE_VIRTUAL(FFaerieItemDataHashInstruction::Hash, return 0; )
};