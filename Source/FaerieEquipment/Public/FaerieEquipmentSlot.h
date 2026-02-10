// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieEquipmentSlotStructs.h"
#include "FaerieItemStackContainer.h"
#include "FaerieSlotTag.h"
#include "FaerieSubObjectFilter.h"
#include "Actions/FaerieInventoryClient.h"
#include "FaerieEquipmentSlot.generated.h"

struct FFaerieAssetInfo;

/**
 * A custom ItemStackContainer that restricts its content according to a Config.
 */
UCLASS(BlueprintType)
class FAERIEEQUIPMENT_API UFaerieEquipmentSlot : public UFaerieItemStackContainer
{
	GENERATED_BODY()

	// We friend the only classes allowed to set our Config
	friend class UFaerieEquipmentManager;
	friend class UFaerieChildSlotToken;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(TMap<FGuid, FInstancedStruct>& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, UFaerieItemContainerExtensionData* ExtensionData) override;

protected:
	FFaerieEquipmentSlotSaveData MakeSlotData(TMap<FGuid, FInstancedStruct>& ExtensionData) const;
	void LoadSlotData(const FFaerieEquipmentSlotSaveData& SlotData, UFaerieItemContainerExtensionData* ExtensionData);
	//~ UFaerieItemContainerBase

public:
	FFaerieSlotTag GetSlotID() const { return Config.SlotID; }

	virtual bool CouldSetInSlot(FFaerieItemStackView View) const override;
	virtual bool CanSetInSlot(FFaerieItemStackView View) const override;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	FFaerieAssetInfo GetSlotInfo() const;

	const UFaerieEquipmentSlot* FindSlot(FFaerieSlotTag SlotTag, bool bRecursive) const;

protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category = "Config", meta = (ExposeOnSpawn = true))
	FFaerieEquipmentSlotConfig Config;
};

namespace Faerie::Equipment
{
	static inline auto SlotFilter = SubObject::Filter().ByClass<UFaerieEquipmentSlot>();
	static inline auto RecursiveSlotFilter = SubObject::Filter().Recursive().ByClass<UFaerieEquipmentSlot>();
}