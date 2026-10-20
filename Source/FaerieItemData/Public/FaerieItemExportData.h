// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "StructUtils/InstancedStruct.h"
#include "FaerieItemExportData.generated.h"

USTRUCT()
struct FAERIEITEMDATA_API FFaerieItemExportData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<const UScriptStruct>> MassTags;

	UPROPERTY()
	TArray<FInstancedStruct> MassInstances;

	UE_REWRITE bool IsEmpty() const { return MassTags.IsEmpty() && MassInstances.IsEmpty(); }
};