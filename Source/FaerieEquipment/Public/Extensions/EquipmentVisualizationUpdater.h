// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "EquipmentVisualizer.h"
#include "ItemContainerExtensionBase.h"
#include "EquipmentVisualizationUpdater.generated.h"

class UFaerieEquipmentSlot;

/**
 *
 */
UCLASS()
class FAERIEEQUIPMENT_API UEquipmentVisualizationUpdater : public UItemContainerExtensionBase
{
	GENERATED_BODY()

protected:
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual void DeinitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;

	virtual void PreRemoval(TNotNull<const UFaerieItemContainerBase*> Container, FEntryKey Key, int32 Removal) override;

	virtual void PostEventBatch(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Inventory::FEventLogBatch& Events) override;

private:
	static UEquipmentVisualizer* GetVisualizer(const UFaerieEquipmentSlot* Slot);

	void CreateVisualForEntry(const UFaerieEquipmentSlot* Slot, FEntryKey Key);
	void RemoveVisualForEntry(const UFaerieEquipmentSlot* Slot, FEntryKey Key);

	void CreateVisualImpl(UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);
	void RemoveVisualImpl(UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);

	struct FPendingAttachment
	{
		FFaerieItemProxy Proxy;
		FEquipmentVisualAttachment Attachment;
	};
	TArray<FPendingAttachment> Pending;
};