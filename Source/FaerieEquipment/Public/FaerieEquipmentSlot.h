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

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ UFaerieItemContainerBase
	virtual FInstancedStruct MakeSaveData(FFaerieItemContainerExtensionData& ExtensionData) const override;
	virtual void LoadSaveData(FConstStructView ItemData, const TSharedStruct<FFaerieItemContainerExtensionData>& ExtensionData) override;

protected:
	FFaerieEquipmentSlotSaveData MakeSlotData(FFaerieItemContainerExtensionData& ExtensionData) const;
	void LoadSlotData(const FFaerieEquipmentSlotSaveData& SlotData, const TSharedStruct<FFaerieItemContainerExtensionData>&  ExtensionData);
	//~ UFaerieItemContainerBase

public:
	FFaerieSlotTag GetSlotID() const { return Config.SlotID; }

	virtual bool CouldSetInSlot(const FFaerieItemDataView& View) const override;
	virtual bool CanSetInSlot(const FFaerieItemDataView& View) const override;

	UFUNCTION(BlueprintCallable, Category = "Faerie|EquipmentSlot")
	FFaerieAssetInfo GetSlotInfo() const;

	const UFaerieEquipmentSlot* FindSlot(const Faerie::ItemData::FRequireEntityManager& EntityManager, FFaerieSlotTag SlotTag, bool bRecursive) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Config", meta = (ExposeOnSpawn = true))
	FFaerieEquipmentSlotConfig Config;
};

namespace Faerie::Equipment
{
	static inline const auto SlotFilter = SubObject::Filter().ByClass<UFaerieEquipmentSlot>();
	static inline const auto RecursiveSlotFilter = SubObject::Filter().Recursive().ByClass<UFaerieEquipmentSlot>();
}