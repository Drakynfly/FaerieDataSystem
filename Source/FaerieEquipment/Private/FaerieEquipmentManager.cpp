// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentManager.h"
#include "EntityManagerHelpers.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlotDescription.h"
#include "FaerieItemStorage.h"

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

	if (!IsTemplate())
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

	for (auto&& Element : InstanceDefaultSlots)
	{
		// Skip adding this default slot if it's been marked as removed (by LoadSaveData).
		if (RemovedDefaultSlots.HasTag(Element.SlotConfig.SlotID))
		{
			continue;
		}

		AddSlot(Element.SlotConfig);
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
	OnEquipmentSlotEventNative.Broadcast(Slot, Event);
	OnEquipmentChangedEvent.Broadcast(Slot, Event);
}

FFaerieEquipmentSaveData UFaerieEquipmentManager::MakeSaveData(const Container::FSaveParams Params) const
{
	FFaerieEquipmentSaveData ManagerSaveData;

	ManagerSaveData.PerSlotData.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		FFaerieSimpleItemStackSaveData& SlotData = ManagerSaveData.PerSlotData.AddDefaulted_GetRef();
		Slot->MakeSaveData(SlotData, Params);
	}
	ManagerSaveData.RemovedDefaultSlots = RemovedDefaultSlots;

	return ManagerSaveData;
}

void UFaerieEquipmentManager::LoadSaveData(const FFaerieEquipmentSaveData& SaveData, const Container::FLoadParams Params)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this);
	Slots.Reset();

	RemovedDefaultSlots = SaveData.RemovedDefaultSlots;
	AddDefaultSlots();

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
			EquipmentSlot = AddSlot(FFaerieEquipmentSlotConfig());
		}
		check(IsValid(EquipmentSlot))

		EquipmentSlot->LoadSaveData(PerSlotDatum, Params);
	}

	// @todo shouldn't we use the SaveData.ExtensionData for our extensions?

	if (IsReadyForReplication())
	{
		AddSubobjectsForReplication();
	}
}

UFaerieItemStackContainer* UFaerieEquipmentManager::AddSlot(const FFaerieEquipmentSlotConfig& Config)
{
	if (!Config.SlotID.IsValid()) return nullptr;

	if (UFaerieItemStackContainer* NewSlot = NewObject<UFaerieItemStackContainer>(this);
		ensure(IsValid(NewSlot)))
	{
		AActor* Owner = GetOwner();

		if (Config.SlotID.IsValid())
		{
			NewSlot->WriteContainerData(FFaerieContainerDataSlotTag::StaticStruct(),
				[Config](const FStructView Data)
				{
					Data.Get<FFaerieContainerDataSlotTag>().SlotTag = Config.SlotID;
				});
		}

		if (IsValid(Config.SlotDescription))
		{
			NewSlot->WriteContainerData(FFaerieItemContainerContentFilter::StaticStruct(),
				[Config](const FStructView Data)
				{
					Data.Get<FFaerieItemContainerContentFilter>().Filter = Config.SlotDescription->Template;
				});
		}

		if (Config.SingleItemSlot)
		{
			NewSlot->WriteContainerData(FFaerieItemContainerCountLimit::StaticStruct(),
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

		NewSlot->SetParentExtensions(this, ExtensionData);

		BroadcastSlotEvent(NewSlot, Equipment::Tags::SlotCreated);

		return NewSlot;
	}

	return nullptr;
}

bool UFaerieEquipmentManager::RemoveSlot(UFaerieItemStackContainer* Slot)
{
	if (IsValid(Slot))
	{
		return false;
	}

	if (Slots.Remove(Slot))
	{
		BroadcastSlotEvent(Slot, Equipment::Tags::SlotDeleted);

		Slot->ClearParentExtensions();

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slot->DeinitializeNetObject(GetOwner());
		GetOwner()->RemoveReplicatedSubObject(Slot);

		Slot->GetOnContainerEvent().RemoveAll(this);

		const FFaerieContainerDataSlotTag& SlotTag = Slot->ReadContainerDataChecked<FFaerieContainerDataSlotTag>();

		// If this slot was a default slot, mark it as removed, so it doesn't get restored after a load.
		for (auto&& Element : InstanceDefaultSlots)
		{
			if (SlotTag.SlotTag == Element.SlotConfig.SlotID)
			{
				RemovedDefaultSlots.AddTag(SlotTag.SlotTag);
				break;
			}
		}

		return true;
	}

	return false;
}

bool UFaerieEquipmentManager::TrySwapSlots(UFaerieItemStackContainer* SlotA, UFaerieItemStackContainer* SlotB)
{
	if (!(IsValid(SlotA) && IsValid(SlotB)))
	{
		return false;
	}

	if (SlotB->IsFilled() && !SlotA->CouldSetInSlot(FFaerieItemProxy(SlotB))) return false;
	if (SlotA->IsFilled() && !SlotB->CouldSetInSlot(FFaerieItemProxy(SlotA))) return false;

	const FFaerieUnownedItemStack ContentA = SlotA->TakeItemFromSlot(ItemData::EntireStack,
															 Inventory::Tags::RemovalMoving);
	const FFaerieUnownedItemStack ContentB = SlotB->TakeItemFromSlot(ItemData::EntireStack,
															 Inventory::Tags::RemovalMoving);

	// Use Impl version to bypass redundant checks to CanSetInSlot
	SlotB->SetStoredItem_Impl(ContentA);
	SlotA->SetStoredItem_Impl(ContentB);

	return true;
}

const UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlotImpl(const UFaerieItemStackContainer* ParentSlot,
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
		const FGameplayTag ChildSlotTag = Child->ReadContainerDataChecked<FFaerieContainerDataSlotTag>().SlotTag;
		if (ChildSlotTag == SlotTag)
		{
			return Child;
		}
	}

	if (bRecursive)
	{
		for (auto&& Child : Children)
		{
			if (auto&& ChildSlot = FindSlotImpl(Child, EntityManager, SlotTag, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

const UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive) const
{
	for (auto&& Slot : Slots)
	{
		if (!IsValid(Slot)) continue;
		const FFaerieContainerDataSlotTag& SlotTag = Slot->ReadContainerDataChecked<FFaerieContainerDataSlotTag>();
		if (SlotTag.SlotTag == SlotID)
		{
			return Slot;
		}
	}

	if (Recursive)
	{
		auto& EntityManager = ItemData::GetFaerieEntityManagerChecked();
		for (auto&& Slot : Slots)
		{
			if (!IsValid(Slot)) continue;
			if (auto&& ChildSlot = FindSlotImpl(Slot, EntityManager, SlotID, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

UFaerieItemStackContainer* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive)
{
	return const_cast<UFaerieItemStackContainer*>(const_cast<const UFaerieEquipmentManager*>(this)->FindSlot(SlotID, Recursive));
}

void UFaerieEquipmentManager::WriteContainerData(const TNotNull<const UScriptStruct*> Type,
	const TFunctionRef<void(FStructView)>& Functor)
{
	Functor(ExtensionData.AddOrGetRef(Type));
}

TArray<FFaerieItemContainerPath> UFaerieEquipmentManager::GetAllContainerPaths() const
{
	SCOPE_CYCLE_COUNTER(STAT_Equipment_BuildPaths);

	auto* EntityManager = ItemData::GetFaerieEntityManager();
	if (!EntityManager)
	{
		// @todo FFaerieItemContainerPath::BuildChildrenPaths doesn't support searching default tokens yet
		return {};
	}

	TArray<FFaerieItemContainerPath> OutPaths;
	OutPaths.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		if (Slot->IsFilled())
		{
			const FFaerieItemInstance Instance = Slot->GetItemInstance().GetValue();
			if (Instance.IsMutable())
			{
				FFaerieItemContainerPath::BuildChildrenPaths(*EntityManager, FFaerieItemProxy(), Slot, OutPaths);
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
