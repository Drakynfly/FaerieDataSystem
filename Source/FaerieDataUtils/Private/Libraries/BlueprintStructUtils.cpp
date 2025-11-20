// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BlueprintStructUtils.h"
#include "StructViewWrapper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlueprintStructUtils)

FFaerieStructViewWrapper UBlueprintStructUtils::ToStructView(const FInstancedStruct& Struct)
{
	// Blueprint doesn't understand const, so don't bother pretending to preserve const-safety...
	return FFaerieStructViewWrapper(FStructView(Struct.GetScriptStruct(), const_cast<uint8*>(Struct.GetMemory())));
}
