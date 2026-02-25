// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStackView.h"
#include "UObject/Object.h"
#include "FaerieItemStackHashInstruction.generated.h"

/**
 * Another command class.
 * Base for hashing functions that take a FaerieItemStackView.
 * Children can be prefixed with "UFISHI_", since this class name is quite long.
 */
UCLASS(Abstract, Const, EditInlineNew, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemStackHashInstruction : public UObject
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(FFaerieItemStackView StackView) const PURE_VIRTUAL(UFaerieItemStackHashInstruction::Hash, return 0; )

protected:
	// Utility for hashing an instruction contained in this one.
	static uint32 ChildHash(const UFaerieItemStackHashInstruction* Child, FFaerieItemStackView StackView);
};