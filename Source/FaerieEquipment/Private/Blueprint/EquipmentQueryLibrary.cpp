// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EquipmentQueryLibrary.h"
#include "DelegateCommon.h"
#include "EntityManagerHelpers.h"
#include "EquipmentHashAsset.h"
#include "EquipmentHashStatics.h"
#include "EquipmentQueryStatics.h"
#include "FaerieEquipmentManager.h"
#include "FaerieHash.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EquipmentQueryLibrary)

bool UFaerieEquipmentLibrary::RunEquipmentQuery(UFaerieEquipmentManager* Manager, const FFaerieEquipmentSetQuery& SetQuery, UFaerieEquipmentSlot*& PassingSlot)
{
	if (!IsValid(Manager))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Manager passed to UFaerieEquipmentLibrary::RunEquipmentQuery"), ELogVerbosity::Error);
		return false;
	}

	return Faerie::Equipment::RunEquipmentQuery(Manager, SetQuery, PassingSlot);
}

FFaerieHash UFaerieEquipmentLibrary::HashEquipment(const UFaerieEquipmentManager* Manager,
												   const FFaerieEquipmentHashConfig& Config)
{
	if (!IsValid(Manager))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Manager passed to UFaerieEquipmentLibrary::HashEquipment"), ELogVerbosity::Error);
		return FFaerieHash();
	}

	if (!Config.HashFunction.IsBound())
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Config.HashFunction passed to UFaerieEquipmentLibrary::HashEquipment"), ELogVerbosity::Error);
		return FFaerieHash();
	}

	auto* EntityManager = Faerie::ItemData::GetFaerieEntityManager();

	return Faerie::Hash::HashEquipment(Manager, EntityManager, Config.Slots,
		[&Config](const FMassEntityManager*, const Faerie::TValid<const FFaerieItemInstance&> Item)
		{
			return Config.HashFunction.Execute(ValidGet(Item));
		});
}

bool UFaerieEquipmentLibrary::ExecuteHashInstructions(const UFaerieEquipmentManager* Manager, const UFaerieEquipmentHashAsset* Asset)
{
	if (!IsValid(Manager))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Manager passed to UFaerieEquipmentLibrary::ExecuteHashInstructions"), ELogVerbosity::Error);
		return false;
	}

	if (!IsValid(Asset))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid Asset passed to UFaerieEquipmentLibrary::ExecuteHashInstructions"), ELogVerbosity::Error);
		return false;
	}

	auto* EntityManager = Faerie::ItemData::GetFaerieEntityManager();

	return Faerie::Hash::ExecuteHashInstructions(Manager, EntityManager, Asset);
}

FBlueprintEquipmentHash UFaerieEquipmentLibrary::GetEquipmentHash_ByName()
{
	return AUTO_DELEGATE_STATIC(FBlueprintEquipmentHash, ThisClass, ExecHashItemByName);
}

int32 UFaerieEquipmentLibrary::ExecHashItemByName(const FFaerieItemInstance& Instance)
{
	if (!Instance.IsValid()) return 0;
	auto* EntityManager = Faerie::ItemData::GetFaerieEntityManager();
	return Faerie::Hash::HashItemByName(EntityManager, Instance);
}