﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStack.h"
#include "UObject/Object.h"
#include "FaerieItemStackHashInstruction.generated.h"

/**
 * Another command class.
 * Base for hashing functions that take a FaerieItemStackView.
 * Children can be prefixed with "UFISHI_", since this class name is quite long.
 */
UCLASS(Const, Abstract, EditInlineNew, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemStackHashInstruction : public UObject
{
	GENERATED_BODY()

public:
	virtual int32 Hash(FFaerieItemStackView StackView) const PURE_VIRTUAL(UFaerieItemStackHashInstruction::Hash, return 0; )

protected:
	// Utility for hashing an instruction contained in this one.
	static int32 ChildHash(const UFaerieItemStackHashInstruction* Child, FFaerieItemStackView StackView);
};