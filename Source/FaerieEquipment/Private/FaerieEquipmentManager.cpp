// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentManager.h"
#include "EntityManagerHelpers.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlotDescription.h"
#include "FaerieItemStorage.h"

#include "Engine/World.h"

#include "Extensions/InventoryContentFilterExtension.h"
#include "Extensions/ItemContainerCountLimit.h"

#include "GameFramework/Actor.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentManager)

DECLARE_STATS_GROUP(TEXT("FaerieEquipmentManager"), STATGROUP_FaerieEquipmentManager, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BuildPaths"), STAT_Equipment_BuildPaths, STATGROUP_FaerieEquipmentManager);

namespace Faerie::Equipment::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, SlotCreated, "Fae.Inventory.SlotCreated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, SlotDeleted, "Fae.Inventory.SlotDeleted")
}

using namespace Faerie;

UFaerieEquipmentManager::UFaerieEquipmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UFaerieEquipmentManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Slots, Params)
}

void UFaerieEquipmentManager::InitializeComponent()
{
	Super::InitializeComponent();

	AddDefaultSlots();
}

void UFaerieEquipmentManager::OnComponentCreated()
{
	Super::OnComponentCreated();

	if (!IsTemplate() && GetWorld()->HasBegunPlay())
	{
		AddDefaultSlots();
	}
}

void UFaerieEquipmentManager::ReadyForReplication()
{
	Super::ReadyForReplication();

	AddSubobjectsForReplication();
}

void UFaerieEquipmentManager::AddDefaultSlots()
{
	if (!Slots.IsEmpty())
	{
		// Default slots already added
		return;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	for (auto&& Element : InstanceDefaultSlots)
	{
		AddSlotImpl(EntityManager, Element.SlotConfig);
	}
}

void UFaerieEquipmentManager::AddSubobjectsForReplication()
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (!Owner->HasAuthority()) return;

	if (!Owner->IsUsingRegisteredSubObjectList())
	{
		UE_LOGF(LogFaerieEquipment, Warning,
			"Owner of Equipment Manager '%ls' does not replicate SubObjectList. Component will not be replicated correctly!", *Owner->GetName())
	}
	else
	{
		for (auto&& Slot : Slots)
		{
			if (IsValid(Slot))
			{
				GetOwner()->AddReplicatedSubObject(Slot);
				Slot->InitializeNetObject(Owner);
			}
		}

		// Make slots replicate once
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
	}
}

void UFaerieEquipmentManager::OnDataChangeEvent(const FFaerieItemProxy& Proxy, const FGameplayTag Tag)
{
	if (UFaerieItemStackContainer* Slot = const_cast<UFaerieItemStackContainer*>(CastChecked<UFaerieItemStackContainer>(Proxy.GetProxyObject())))
	{
		BroadcastSlotEvent(Slot, Inventory::Tags::ReplicationEdit);
	}
}

void UFaerieEquipmentManager::BroadcastSlotEvent(const TNotNull<UFaerieItemStackContainer*> Slot, const FFaerieInventoryTag Event)
{
	OnEquipmentSlotEventNative.Broadcast(FFaerieItemProxy(Slot), Event);
	OnEquipmentChangedEvent.Broadcast(Slot, Event);
}

UFaerieItemStackContainer* UFaerieEquipmentManager::AddSlotImpl(FMassEntityManager& EntityManager, const FFaerieEquipmentSlotConfig& Config)
{
	if (UFaerieItemStackContainer* NewSlot = NewObject<UFaerieItemStackContainer>(this);
		ensure(IsValid(NewSlot)))
	{
		AActor* Owner = GetOwner();

		if (Config.SlotID.IsValid())
		{
			NewSlot->WriteContainerData(EntityManager, FFaerieContainerDataSlotTag::StaticStruct(),
				[Config](const FStructView Data)
				{
					Data.Get<FFaerieContainerDataSlotTag>().SlotTag = Config.SlotID;
				});
		}

		if (IsValid(Config.SlotDescription))
		{
			NewSlot->WriteContainerData(EntityManager, FFaerieItemContainerContentFilter::StaticStruct(),
				[Config](const FStructView Data)
				{
					Data.Get<FFaerieItemContainerContentFilter>().Filter = Config.SlotDescription->Template;
				});
		}

		if (Config.SingleItemSlot)
		{
			NewSlot->WriteContainerData(EntityManager, FFaerieItemContainerCountLimit::StaticStruct(),
				[](const FStructView Data)
				{
					Data.Get<FFaerieItemContainerCountLimit>().SetMaxInstanceCount(1);
				});
		}

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slots.Add(NewSlot);
		Owner->AddReplicatedSubObject(NewSlot);
		NewSlot->InitializeNetObject(Owner);

		NewSlot->GetOnContainerEvent().AddUObject(this, &ThisClass::OnDataChangeEvent);

		NewSlot->SetParentExtensions(this);

		BroadcastSlotEvent(NewSlot, Equipment::Tags::SlotCreated);

		return NewSlot;
	}

	return nullptr;
}

const UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlotImpl(const FMassEntityManager& EntityManager, const FFaerieSlotTag SlotTag, const bool bRecursive) const
{
	for (auto&& Slot : Slots)
	{
		if (!IsValid(Slot)) continue;
		const FFaerieContainerDataSlotTag& DataSlotTag = Slot->ReadContainerDataChecked<FFaerieContainerDataSlotTag>(EntityManager);
		if (DataSlotTag.SlotTag == SlotTag)
		{
			return Slot;
		}
	}

	if (bRecursive)
	{
		for (auto&& Slot : Slots)
		{
			if (!IsValid(Slot)) continue;
			if (auto&& ChildSlot = FindSubSlotImpl(Slot, EntityManager, SlotTag, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

const UFaerieItemStackContainer* UFaerieEquipmentManager::FindSubSlotImpl(const UFaerieItemStackContainer* ParentSlot,
																		  const FMassEntityManager& EntityManager, const FFaerieSlotTag SlotTag, const bool bRecursive)
{
	if (!ParentSlot->IsFilled())
	{
		return nullptr;
	}

	const FFaerieItemInstance SlotInstance = ParentSlot->GetItemInstance().GetValue();
	if (!SlotInstance.IsMutable())
	{
		// If the instance is not mutable, it will never contain children.
		return nullptr;
	}

	TArray<TNotNull<UFaerieItemStackContainer*>> Children;
	Equipment::SlotFilter.Emit(EntityManager, SlotInstance, Children);

	for (auto&& Child : Children)
	{
		const FGameplayTag ChildSlotTag = Child->ReadContainerDataChecked<FFaerieContainerDataSlotTag>(EntityManager).SlotTag;
		if (ChildSlotTag == SlotTag)
		{
			return Child;
		}
	}

	if (bRecursive)
	{
		for (auto&& Child : Children)
		{
			if (auto&& ChildSlot = FindSubSlotImpl(Child, EntityManager, SlotTag, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

FFaerieEquipmentSaveData UFaerieEquipmentManager::MakeSaveData(const FMassEntityManager& EntityManager, const Container::FSaveParams Params) const
{
	FFaerieEquipmentSaveData ManagerSaveData;

	ManagerSaveData.PerSlotData.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		FFaerieSimpleItemStackSaveData& SlotData = ManagerSaveData.PerSlotData.AddDefaulted_GetRef();
		Slot->MakeSaveData(EntityManager, SlotData, Params);
	}

	return ManagerSaveData;
}

void UFaerieEquipmentManager::LoadSaveData(FMassEntityManager& EntityManager, const FFaerieEquipmentSaveData& SaveData, const Container::FLoadParams Params)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this);
	Slots.Reset();

	for (const FFaerieSimpleItemStackSaveData& PerSlotDatum : SaveData.PerSlotData)
	{
		FFaerieSlotTag SlotID;
		for (auto&& Element : PerSlotDatum.ExtensionData)
		{
			if (auto&& AsSlotTag = Element.GetPtr<FFaerieContainerDataSlotTag>())
			{
				SlotID = AsSlotTag->SlotTag;
			}
		}
		if (!SlotID.IsValid()) continue;

		// Find or create slot for this tag.
		UFaerieItemStackContainer* EquipmentSlot = FindSlot(SlotID);
		if (!IsValid(EquipmentSlot))
		{
			EquipmentSlot = AddSlotImpl(EntityManager, FFaerieEquipmentSlotConfig());
		}
		check(IsValid(EquipmentSlot))

		EquipmentSlot->LoadSaveData(EntityManager, PerSlotDatum, Params);
	}

	// @todo shouldn't we use the SaveData.ExtensionData for our extensions?

	if (IsReadyForReplication())
	{
		AddSubobjectsForReplication();
	}
}

void UFaerieEquipmentManager::AddSlot(const FFaerieEquipmentSlotConfig& Config)
{
	if (!Config.SlotID.IsValid()) return;
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	AddSlotImpl(EntityManager, Config);
}

bool UFaerieEquipmentManager::RemoveSlot(const FFaerieSlotTag Slot)
{
	UFaerieItemStackContainer* Slot_Obj = FindSlot(Slot, true);
	if (!IsValid(Slot_Obj))
	{
		return false;
	}

	if (Slots.Remove(Slot_Obj))
	{
		BroadcastSlotEvent(Slot_Obj, Equipment::Tags::SlotDeleted);

		Slot_Obj->ClearParentExtensions();

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slot_Obj->DeinitializeNetObject(GetOwner());
		GetOwner()->RemoveReplicatedSubObject(Slot_Obj);

		Slot_Obj->GetOnContainerEvent().RemoveAll(this);

		return true;
	}

	return false;
}

bool UFaerieEquipmentManager::TrySwapSlots(const FFaerieSlotTag SlotA, const FFaerieSlotTag SlotB)
{
	UFaerieItemStackContainer* SlotA_Obj = FindSlot(SlotA, true);
	UFaerieItemStackContainer* SlotB_Obj = FindSlot(SlotB, true);
	if (!IsValid(SlotA_Obj) || !IsValid(SlotB_Obj))
	{
		return false;
	}

	if (SlotB_Obj->IsFilled() && !SlotA_Obj->CouldSetInSlot(FFaerieItemProxy(SlotB_Obj))) return false;
	if (SlotA_Obj->IsFilled() && !SlotB_Obj->CouldSetInSlot(FFaerieItemProxy(SlotA_Obj))) return false;

	const FFaerieUnownedItemStack ContentA = SlotA_Obj->TakeItemFromSlot(ItemData::EntireStack,
															 Inventory::Tags::RemovalMoving);
	const FFaerieUnownedItemStack ContentB = SlotB_Obj->TakeItemFromSlot(ItemData::EntireStack,
															 Inventory::Tags::RemovalMoving);

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	// Use Impl version to bypass redundant checks to CanSetInSlot
	if (ContentA.IsValid())
	{
		SlotB_Obj->SetStoredItem_Impl(EntityManager, ContentA);
	}
	if (ContentB.IsValid())
	{
		SlotA_Obj->SetStoredItem_Impl(EntityManager, ContentB);
	}

	return true;
}

const UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive) const
{
	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return FindSlotImpl(EntityManager, SlotID, Recursive);
}

UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive)
{
	return const_cast<UFaerieItemStackContainer*>(const_cast<const UFaerieEquipmentManager*>(this)->FindSlot(SlotID, Recursive));
}

FFaerieItemProxy UFaerieEquipmentManager::FindSlotProxy(const FFaerieSlotTag SlotID, const bool Recursive) const
{
	auto& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	return FFaerieItemProxy(FindSlotImpl(EntityManager, SlotID, Recursive));
}

void UFaerieEquipmentManager::WriteContainerData(FMassEntityManager& EntityManager, const TNotNull<const UScriptStruct*> Type,
												 const TFunctionRef<void(FStructView)>& Functor)
{
	Functor(ExtensionData.AddOrGetRef(EntityManager, Type));
}

TArray<FFaerieItemContainerPath> UFaerieEquipmentManager::GetAllContainerPaths() const
{
	SCOPE_CYCLE_COUNTER(STAT_Equipment_BuildPaths);

	const UWorld* World = GetWorld();
	if (!ItemData::HasFaerieEntityManagerBeenAssigned(World))
	{
		FFrame::KismetExecutionMessage(TEXT("No Entity Manager assigned to handle UFaerieEquipmentManager::GetAllContainerPaths"), ELogVerbosity::Error);
		return {};
	}

	const FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(World);

	TArray<FFaerieItemContainerPath> OutPaths;
	OutPaths.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		if (Slot->IsFilled())
		{
			const FFaerieItemInstance Instance = Slot->GetItemInstance().GetValue();
			if (Instance.IsMutable())
			{
				FFaerieItemContainerPath::BuildChildrenPaths(EntityManager, FFaerieItemProxy(), Slot, OutPaths);
			}
		}
	}

	return OutPaths;
}

void UFaerieEquipmentManager::PrintSlotDebugInfo() const
{
#if !UE_BUILD_SHIPPING
	for (auto&& Slot : Slots)
	{
		///
	}
#endif
}
