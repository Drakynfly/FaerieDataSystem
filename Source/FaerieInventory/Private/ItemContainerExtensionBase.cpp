// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ItemContainerExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemContainerExtensionBase)

using namespace Faerie;

void FFaerieItemContainerData::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		const UScriptStruct* ActualType = GetScriptStruct();
		Ar.MarkSearchableName(FFaerieItemContainerData::StaticStruct(), *ActualType->GetName());
	}
}