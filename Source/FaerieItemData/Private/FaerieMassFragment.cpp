// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieMassFragment.h"

#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassFragment)

// Returning false will report to UScriptStruct::SerializeItem that it should fallback to default serialization logic.
// That is fine, as we are only using this as a point to inject the searchable name.
#define FAERIE_IMPLEMENT_SERIALIZE(Type)\
bool Type::Serialize(FArchive& Ar, const UStruct* DefaultsStruct, const void* Defaults)\
{\
	if (Ar.IsSaving())\
	{\
		Ar.MarkSearchableName(Type::StaticStruct(), *DefaultsStruct->GetName());\
	}\
	return false;\
}

FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSparseFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassTag)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSparseTag)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassSharedFragment)
FAERIE_IMPLEMENT_SERIALIZE(FFaerieMassConstSharedFragment)

namespace Faerie::ItemData
{
	bool IsFaerieMassType(const TNotNull<const UScriptStruct*> Type)
	{
		return Type->IsChildOf<FFaerieMassFragment>() ||
			Type->IsChildOf<FFaerieMassSparseFragment>() ||
			Type->IsChildOf<FFaerieMassTag>() ||
			Type->IsChildOf<FFaerieMassSparseTag>() ||
			Type->IsChildOf<FFaerieMassSharedFragment>() ||
			Type->IsChildOf<FFaerieMassConstSharedFragment>();
	}

	bool IsFaerieMassTag(const TNotNull<const UScriptStruct*> Type)
	{
		return Type->IsChildOf<FFaerieMassTag>() ||
			Type->IsChildOf<FFaerieMassSparseTag>();
	}
}