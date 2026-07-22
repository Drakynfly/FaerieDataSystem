// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemMutator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemMutator)

void FFaerieItemMutator::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		const UScriptStruct* ActualType = GetScriptStruct();
		Ar.MarkSearchableName(FFaerieItemMutator::StaticStruct(), *ActualType->GetName());
	}
}
