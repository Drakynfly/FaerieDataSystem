﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "EquipmentManagerInterface.generated.h"

class UFaerieEquipmentManager;

UINTERFACE(MinimalAPI)
class UEquipmentManagerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class FAERIEEQUIPMENT_API IEquipmentManagerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment|Redirector")
	UFaerieEquipmentManager* ResolveManager() const;
};