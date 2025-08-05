// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "EquipmentVisualizer.h"
#include "ItemContainerExtensionBase.h"
#include "EquipmentVisualizationUpdater.generated.h"

class UVisualSlotExtension;

/**
 *
 */
UCLASS()
class FAERIEEQUIPMENT_API UEquipmentVisualizationUpdater : public UItemContainerExtensionBase
{
	GENERATED_BODY()

protected:
	virtual void InitializeExtension(const UFaerieItemContainerBase* Container) override;
	virtual void DeinitializeExtension(const UFaerieItemContainerBase* Container) override;

	virtual void PostAddition(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;
	virtual void PreRemoval(const UFaerieItemContainerBase* Container, FEntryKey Key, int32 Removal) override;
	virtual void PostEntryChanged(const UFaerieItemContainerBase* Container, const Faerie::Inventory::FEventLog& Event) override;

private:
	static UEquipmentVisualizer* GetVisualizer(const UFaerieItemContainerBase* Container);

	static FEquipmentVisualAttachment FindAttachmentParent(const UFaerieItemContainerBase* Container, const UVisualSlotExtension*& SlotExtension, const UEquipmentVisualizer* Visualizer);

	void CreateVisualForEntry(const UFaerieItemContainerBase* Container, FEntryKey Key);
	void RemoveVisualForEntry(const UFaerieItemContainerBase* Container, FEntryKey Key);

	void CreateVisualImpl(const UFaerieItemContainerBase* Container, UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);
	void RemoveVisualImpl(UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);

	TMultiMap<TWeakObjectPtr<const UFaerieItemContainerBase>, FEntryKey> SpawnKeys;

	struct FPendingAttachment
	{
		FFaerieItemProxy Proxy;
		FEquipmentVisualAttachment Attachment;
	};
	TArray<FPendingAttachment> Pending;
};