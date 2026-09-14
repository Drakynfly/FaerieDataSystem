// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EquipmentExtensionsLibrary.generated.h"

class UFaerieContainerContentHashView;
class UFaerieItemContainerCapacityView;
class UFaerieEquipmentManager;
class UFaerieVisualSlotConfiguration;
class UFaerieItemContainerBase;
struct FFaerieSlotTag;

/**
 * 
 */
UCLASS()
class FAERIEEQUIPMENT_API UFaerieEquipmentExtensionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Gets the identifying slot tag associated with a container.
	UFUNCTION(BlueprintPure, Category = "Faerie")
	static FFaerieSlotTag GetContainerSlotTag(UFaerieItemContainerBase* Container);

	UFUNCTION(BlueprintPure, Category = "Faerie")
	UFaerieVisualSlotConfiguration* GetVisualSlotConfiguration(UFaerieItemContainerBase* Container);

	// @Todo temporary shunt while equipment manager cannot use views correctly
	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static UFaerieItemContainerCapacityView* GetOrCreateCapacityView_EquipmentManager(UFaerieEquipmentManager* Manager);

	// @Todo temporary shunt while equipment manager cannot use views correctly
	UFUNCTION(BlueprintCallable, Category = "Faerie|CommonExtensionUtils")
	static UFaerieContainerContentHashView* GetOrCreateContentHashView_EquipmentManager(UFaerieEquipmentManager* Manager);
};
