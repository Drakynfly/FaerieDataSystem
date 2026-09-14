// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieCommonExtensionUtils.h"
#include "EntityManagerHelpers.h"
#include "FaerieItemContainerBase.h"
#include "FaerieItemStorage.h"

#include "Capacity/InventoryCapacityExtension.h"

#include "Extensions/ContentHashExtension.h"
#include "Extensions/InventoryLoggerExtension.h"
#include "Extensions/ItemContainerCountLimit.h"

#include "GridLayout/InventoryGridExtensionBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCommonExtensionUtils)

using namespace Faerie;

int32 UFaerieCommonExtensionUtils::GetContainerStackLimit(UFaerieItemContainerBase* Container)
{
	if (const FConstStructView LimitData = Container->ReadContainerData(
		FFaerieItemContainerCountLimit::StaticStruct(), true);
		LimitData.IsValid())
	{
		return LimitData.Get<FFaerieItemContainerCountLimit>().GetMaxInstanceCount();
	}
	return ItemData::UnlimitedStack;
}

UFaerieItemContainerCapacityView* UFaerieCommonExtensionUtils::GetOrCreateCapacityView(
	UFaerieItemContainerBase* Container)
{
	if (!IsValid(Container))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Container passed to UFaerieCommonExtensionUtils::GetOrCreateCapacityView"), ELogVerbosity::Error);
		return nullptr;
	}

	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieCommonExtensionUtils::GetOrCreateCapacityView"), ELogVerbosity::Error);
		return nullptr;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	UFaerieItemContainerCapacityView* View = NewObject<UFaerieItemContainerCapacityView>();
	View->InitializeView(EntityManager, Content::FCapacityViewFragment::StaticStruct(), MakeTuple(Container, &Container->GetExtensionData()));

	return View;
}

UFaerieContainerEventLogView* UFaerieCommonExtensionUtils::GetOrCreateContainerEventLogView(
	UFaerieItemContainerBase* Container)
{
	if (!IsValid(Container))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Container passed to UFaerieCommonExtensionUtils::GetOrCreateContainerEventLogView"), ELogVerbosity::Error);
		return nullptr;
	}

	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieCommonExtensionUtils::GetOrCreateContainerEventLogView"), ELogVerbosity::Error);
		return nullptr;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	UFaerieContainerEventLogView* View = NewObject<UFaerieContainerEventLogView>();
	View->InitializeView(EntityManager, Content::FEventLogViewFragment::StaticStruct(), MakeTuple(Container, &Container->GetExtensionData()));

	return View;
}

UFaerieContainerContentHashView* UFaerieCommonExtensionUtils::GetOrCreateContainerHashView(
	UFaerieItemContainerBase* Container)
{
	if (!IsValid(Container))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Container passed to UFaerieCommonExtensionUtils::GetOrCreateContainerEventLogView"), ELogVerbosity::Error);
		return nullptr;
	}

	if (!ItemData::HasFaerieEntityManagerBeenAssigned())
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentExtensionsLibrary::GetOrCreateContentHashView_EquipmentManager"), ELogVerbosity::Error);
		return nullptr;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked();

	UFaerieContainerContentHashView* View = NewObject<UFaerieContainerContentHashView>();
	View->InitializeView(EntityManager, Content::FContentHashViewFragment::StaticStruct(), MakeTuple(Container, &Container->GetExtensionData()));

	return View;
}

FFaerieItemProxy UFaerieCommonExtensionUtils::ViewAt(const FFaerieContainerGridReadContext& GridContext,
	const FIntPoint& Position)
{
	return GridContext.ViewAt(Position);
}

bool UFaerieCommonExtensionUtils::IsCellOccupied(const FFaerieContainerGridReadContext& GridContext,
	const FIntPoint& Point)
{
	return GridContext.IsCellOccupied(Point);
}

FFaerieGridPlacement UFaerieCommonExtensionUtils::GetStackPlacementData(
	const FFaerieContainerGridReadContext& GridContext, const FFaerieAddress Address)
{
	return GridContext.GetStackPlacementData(Address);
}
