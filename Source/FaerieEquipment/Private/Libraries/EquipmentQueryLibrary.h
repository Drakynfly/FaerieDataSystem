// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EquipmentQueryLibrary.generated.h"

struct FFaerieEquipmentSetQuery;
class UFaerieEquipmentManager;
class UFaerieEquipmentSlot;

/**
 *
 */
UCLASS()
class UFaerieEquipmentQueryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentQuery")
	static bool RunEquipmentQuery(UFaerieEquipmentManager* Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot);
};