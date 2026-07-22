// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieSlotTag.h"
#include "FaerieEquipmentSlotStructs.generated.h"

class UFaerieItem;
class UFaerieEquipmentSlotDescription;

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

USTRUCT()
struct FFaerieEquipmentSlotSaveData
{
	GENERATED_BODY()

	// The ID of the configured slot this save data belongs to.
	UPROPERTY()
	FFaerieSlotTag SlotID;

	UPROPERTY()
	TObjectPtr<const UFaerieItem> ItemObject;

	UPROPERTY()
	int32 Copies = 0;

	// Additional data stored with this item instance.
	UPROPERTY()
	FFaerieItemExportData ExportData;
};