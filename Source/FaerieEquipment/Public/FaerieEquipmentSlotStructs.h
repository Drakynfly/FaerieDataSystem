// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieSlotTag.h"
#include "ItemContainerExtensionBase.h"
#include "FaerieEquipmentSlotStructs.generated.h"

class UFaerieEquipmentSlotDescription;

USTRUCT()
struct FFaerieContainerDataSlotTag : public FFaerieItemContainerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ContainerDataSlotTag")
	FFaerieSlotTag SlotTag;

____FAERIE_CONTAINER_DATA_DECL(FFaerieContainerDataSlotTag)
};

USTRUCT(BlueprintType)
struct FFaerieEquipmentSlotConfig
{
	GENERATED_BODY()

	// Unique ID for this slot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FFaerieSlotTag SlotID;

	// Info about this slot.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UFaerieEquipmentSlotDescription> SlotDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool SingleItemSlot = true;
};