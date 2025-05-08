// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ItemContainerExtensionBase.h"
#include "EquipmentVisualizationUpdater.generated.h"

class UEquipmentVisualizer;

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

	void CreateNewVisual(const UFaerieItemContainerBase* Container, FEntryKey Key);
	void RemoveOldVisual(const UFaerieItemContainerBase* Container, FEntryKey Key);

	static void CreateNewVisualImpl(const UFaerieItemContainerBase* Container, UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);
	static void RemoveOldVisualImpl(UEquipmentVisualizer* Visualizer, FFaerieItemProxy Proxy);

	TMultiMap<TWeakObjectPtr<const UFaerieItemContainerBase>, FEntryKey> SpawnKeys;
};