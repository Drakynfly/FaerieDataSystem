// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintStructUtils.generated.h"

struct FFaerieStructViewWrapper;
struct FInstancedStruct;

/**
 * 
 */
UCLASS()
class UBlueprintStructUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Faerie|Utils", meta = (BlueprintAutocast, CompactNodeTitle = "->"))
	static FFaerieStructViewWrapper ToStructView(const FInstancedStruct& Struct);
};
