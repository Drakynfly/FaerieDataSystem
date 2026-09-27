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
	FMassEntityManager& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked(Container->GetWorld());
	if (const FConstStructView TagData = Container->ReadContainerData(EntityManager,
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
	FMassEntityManager& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked(Container->GetWorld());
	if (const FConstStructView VisualUpdater = Container->ReadContainerData(EntityManager,
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

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned(Manager->GetWorld()))
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentExtensionsLibrary::GetOrCreateCapacityView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	auto& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked(Manager->GetWorld());

	UFaerieItemContainerCapacityView* View = NewObject<UFaerieItemContainerCapacityView>(Manager);
	View->InitializeView(EntityManager, Faerie::Content::FContainerCapacityViewFragment::StaticStruct(), Manager);

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

	if (!Faerie::ItemData::HasFaerieEntityManagerBeenAssigned(Manager->GetWorld()))
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentExtensionsLibrary::GetOrCreateContentHashView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	auto& EntityManager = Faerie::ItemData::GetFaerieEntityManagerChecked(Manager->GetWorld());

	UFaerieContainerContentHashView* View = NewObject<UFaerieContainerContentHashView>(Manager);
	View->InitializeView(EntityManager, Faerie::Content::FContentHashViewFragment::StaticStruct(), Manager);

	return View;
}
