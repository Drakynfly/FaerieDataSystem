// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentExtensionsLibrary.h"
#include "EntityManagerHelpers.h"
#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentSlotStructs.h"
#include "FaerieItemContainerBase.h"

#include "Capacity/InventoryCapacityExtension.h"

#include "Extensions/ContentHashExtension.h"

#include "Visualizer/EquipmentVisualizationUpdater.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentExtensionsLibrary)

FFaerieSlotTag UFaerieEquipmentExtensionsLibrary::GetContainerSlotTag(UFaerieItemContainerBase* Container)
{
	if (const FConstStructView TagData = Container->ReadContainerData(
		FFaerieContainerDataSlotTag::StaticStruct());
		TagData.IsValid())
	{
		return TagData.Get<FFaerieContainerDataSlotTag>().SlotTag;
	}
	return FFaerieSlotTag();
}

UFaerieVisualSlotConfiguration* UFaerieEquipmentExtensionsLibrary::GetVisualSlotConfiguration(
	UFaerieItemContainerBase* Container)
{
	if (const FConstStructView VisualUpdater = Container->ReadContainerData(
		FFaerieContainerExtensionVisualUpdater::StaticStruct(), true);
		VisualUpdater.IsValid())
	{
		return VisualUpdater.Get<FFaerieContainerExtensionVisualUpdater>().Config;
	}
	return nullptr;
}

UFaerieItemContainerCapacityView* UFaerieEquipmentExtensionsLibrary::GetOrCreateCapacityView_EquipmentManager(
	UFaerieEquipmentManager* Manager)
{
	if (!IsValid(Manager))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Manager passed to UFaerieEquipmentExtensionsLibrary::GetOrCreateCapacityView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentExtensionsLibrary::GetOrCreateCapacityView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	auto& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked();

	UFaerieItemContainerCapacityView* View = NewObject<UFaerieItemContainerCapacityView>();
	View->InitializeView(EntityManager, Faerie::Content::FCapacityViewFragment::StaticStruct(), MakeTuple(Manager, &Manager->GetExtensionData()));

	return View;
}

UFaerieContainerContentHashView* UFaerieEquipmentExtensionsLibrary::GetOrCreateContentHashView_EquipmentManager(
	UFaerieEquipmentManager* Manager)
{
	if (!IsValid(Manager))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Manager passed to UFaerieEquipmentExtensionsLibrary::GetOrCreateContentHashView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentExtensionsLibrary::GetOrCreateContentHashView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	auto& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked();

	UFaerieContainerContentHashView* View = NewObject<UFaerieContainerContentHashView>();
	View->InitializeView(EntityManager, Faerie::Content::FContentHashViewFragment::StaticStruct(), MakeTuple(Manager, &Manager->GetExtensionData()));

	return View;
}
