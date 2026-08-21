// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieGuidFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieGuidFragment)

FAERIE_REGISTER_TRAITS(FFaerieGuidFragment)

bool FFaerieGuidFragment::InitializeRuntime(TNotNull<UObject*> Outer, const FFaerieItemInstance& Instance)
{
	Guid = FGuid::NewGuid();
	return true;
}