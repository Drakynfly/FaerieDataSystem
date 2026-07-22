// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "FaerieItemStackHashInstruction.generated.h"

namespace Faerie::ItemData
{
	struct FValidatedDataView;
}

/**
 * Another command class.
 * Children can be prefixed with "UFISHI_", since this class name is quite long.
 */
UCLASS(Abstract, Const, EditInlineNew, CollapseCategories)
class FAERIEITEMDATA_API UFaerieItemStackHashInstruction : public UObject
{
	GENERATED_BODY()

public:
	virtual uint32 Hash(TNotNull<const UObject*> WorldContextObj, const Faerie::ItemData::FValidatedDataView& View) const PURE_VIRTUAL(UFaerieItemStackHashInstruction::Hash, return 0; )
};