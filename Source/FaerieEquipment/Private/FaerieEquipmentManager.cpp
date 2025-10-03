// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Tokens/FaerieItemStorageToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentManager)

DECLARE_STATS_GROUP(TEXT("FaerieEquipmentManager"), STATGROUP_FaerieEquipmentManager, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("BuildPaths"), STAT_Equipment_BuildPaths, STATGROUP_FaerieEquipmentManager);

namespace Faerie::Equipment::Tags
{
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, SlotCreated, "Fae.Inventory.SlotCreated")
	UE_DEFINE_GAMEPLAY_TAG_TYPED(FFaerieInventoryTag, SlotDeleted, "Fae.Inventory.SlotDeleted")
}

UFaerieEquipmentManager::UFaerieEquipmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	ExtensionGroup = CreateDefaultSubobject<UItemContainerExtensionGroup>("ExtensionGroup");
	SET_NEW_IDENTIFIER(ExtensionGroup, TEXTVIEW("EquipmentManagerGroup"))
}

void UFaerieEquipmentManager::PostInitProperties()
{
	Super::PostInitProperties();

	for (auto&& DefaultSlot : InstanceDefaultSlots)
	{
		if (IsValid(DefaultSlot.ExtensionGroup))
		{
			SET_NEW_IDENTIFIER(DefaultSlot.ExtensionGroup, TEXTVIEW("EquipmentManagerInstanceDefaultSlot"))
		}
	}
}

void UFaerieEquipmentManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Slots, Params);
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

UItemContainerExtensionGroup* UFaerieEquipmentManager::GetExtensionGroup() const
{
	return ExtensionGroup;
}

void UFaerieEquipmentManager::AddDefaultSlots()
{
	if (!Slots.IsEmpty())
	{
		// Default slots already added
		return;
	}

	// Wipe load flags from Extensions. Hack to make replication work :/
	ExtensionGroup->ReplicationFixup();

	for (auto&& Element : InstanceDefaultSlots)
	{
		// Skip adding this default slot if it's been marked as removed (by LoadSaveData).
		if (RemovedDefaultSlots.HasTag(Element.SlotConfig.SlotID))
		{
			continue;
		}

		auto&& DefaultSlot = AddSlot(Element.SlotConfig);
		if (!IsValid(DefaultSlot))
		{
			continue;
		}

		if (IsValid(Element.ExtensionGroup))
		{
			// The default ExtensionGroups are "Assets" in that they are default instances baked into the component, and
			// need to be fixed before they can replicate.
			Element.ExtensionGroup->ReplicationFixup();
			DefaultSlot->AddExtension(Element.ExtensionGroup.Get());
		}
	}
}

void UFaerieEquipmentManager::AddSubobjectsForReplication()
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (!Owner->HasAuthority()) return;

	if (!Owner->IsUsingRegisteredSubObjectList())
	{
		UE_LOG(LogFaerieEquipment, Warning,
			TEXT("Owner of Equipment Manager '%s' does not replicate SubObjectList. Component will not be replicated correctly!"), *Owner->GetName())
	}
	else
	{
		GetOwner()->AddReplicatedSubObject(ExtensionGroup);
		ExtensionGroup->InitializeNetObject(Owner);

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

void UFaerieEquipmentManager::BroadcastSlotEvent(UFaerieItemStackContainer* Container, const FFaerieInventoryTag Event)
{
	if (UFaerieEquipmentSlot* Slot = CastChecked<UFaerieEquipmentSlot>(Container))
	{
		OnEquipmentSlotEventNative.Broadcast(Slot, Event);
		OnEquipmentChangedEvent.Broadcast(Slot, Event);
	}
}

FFaerieEquipmentSaveData UFaerieEquipmentManager::MakeSaveData() const
{
	FFaerieEquipmentSaveData SlotSaveData;

	SlotSaveData.PerSlotData.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		SlotSaveData.PerSlotData.Add(Slot->MakeSlotData(SlotSaveData.ExtensionData));
	}
	SlotSaveData.RemovedDefaultSlots = RemovedDefaultSlots;

	return SlotSaveData;
}

void UFaerieEquipmentManager::LoadSaveData(const FFaerieEquipmentSaveData& SaveData)
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this);
	Slots.Reset();

	RemovedDefaultSlots = SaveData.RemovedDefaultSlots;
	AddDefaultSlots();

	// Construct container to hold extension data for later retrieval.
	UFaerieItemContainerExtensionData* ExtensionData = NewObject<UFaerieItemContainerExtensionData>(this);
	ExtensionData->Data = SaveData.ExtensionData;

	for (const FFaerieEquipmentSlotSaveData& PerSlotDatum : SaveData.PerSlotData)
	{
		UFaerieEquipmentSlot* EquipmentSlot = FindSlot(PerSlotDatum.Config.SlotID);
		if (!IsValid(EquipmentSlot))
		{
			EquipmentSlot = AddSlot(PerSlotDatum.Config);
		}

		if (IsValid(EquipmentSlot))
		{
			EquipmentSlot->LoadSlotData(PerSlotDatum, ExtensionData);
		}
	}

	// @todo shouldn't we use the SaveData.ExtensionData for our extensions?

	if (IsReadyForReplication())
	{
		AddSubobjectsForReplication();
	}
}

UFaerieEquipmentSlot* UFaerieEquipmentManager::AddSlot(const FFaerieEquipmentSlotConfig& Config)
{
	if (!Config.SlotID.IsValid()) return nullptr;
	if (Config.SlotDescription == nullptr) return nullptr;

	if (UFaerieEquipmentSlot* NewSlot = NewObject<UFaerieEquipmentSlot>(this);
		ensure(IsValid(NewSlot)))
	{
		AActor* Owner = GetOwner();

		NewSlot->Config = Config;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slots.Add(NewSlot);
		Owner->AddReplicatedSubObject(NewSlot);
		NewSlot->InitializeNetObject(Owner);

		NewSlot->OnItemChangedNative.AddUObject(this, &ThisClass::BroadcastSlotEvent);

		NewSlot->GetExtensionGroup()->SetParentGroup(ExtensionGroup);

		BroadcastSlotEvent(NewSlot, Faerie::Equipment::Tags::SlotCreated);

		return NewSlot;
	}

	return nullptr;
}

bool UFaerieEquipmentManager::RemoveSlot(UFaerieEquipmentSlot* Slot)
{
	if (IsValid(Slot))
	{
		return false;
	}

	if (Slots.Remove(Slot))
	{
		BroadcastSlotEvent(Slot, Faerie::Equipment::Tags::SlotDeleted);

		Slot->GetExtensionGroup()->SetParentGroup(nullptr);

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slot->DeinitializeNetObject(GetOwner());
		GetOwner()->RemoveReplicatedSubObject(Slot);

		Slot->OnItemChangedNative.RemoveAll(this);

		// If this slot was a default slot, mark it as removed, so it doesn't get restored after a load.
		for (auto&& Element : InstanceDefaultSlots)
		{
			if (Slot->GetSlotID() == Element.SlotConfig.SlotID)
			{
				RemovedDefaultSlots.AddTag(Slot->GetSlotID());
				break;
			}
		}

		return true;
	}

	return false;
}

UFaerieEquipmentSlot* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive) const
{
	for (auto&& Slot : Slots)
	{
		if (!IsValid(Slot)) continue;
		if (Slot->GetSlotID() == SlotID)
		{
			return Slot;
		}
	}

	if (Recursive)
	{
		for (auto&& Slot : Slots)
		{
			if (!IsValid(Slot)) continue;
			if (auto&& ChildSlot = Slot->FindSlot(SlotID, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

bool UFaerieEquipmentManager::AddExtension(UItemContainerExtensionBase* Extension)
{
	if (ExtensionGroup->AddExtension(Extension))
	{
		GetOwner()->AddReplicatedSubObject(Extension);
		Extension->InitializeNetObject(GetOwner());
		return true;
	}
	return false;
}

bool UFaerieEquipmentManager::RemoveExtension(UItemContainerExtensionBase* Extension)
{
	if (!ensure(IsValid(Extension)))
	{
		return false;
	}

	Extension->DeinitializeNetObject(GetOwner());
	GetOwner()->RemoveReplicatedSubObject(Extension);
	return ExtensionGroup->RemoveExtension(Extension);
}

bool UFaerieEquipmentManager::CanClientRunActions(const UFaerieInventoryClient* Client) const
{
	// @todo implement permissions
	return true;
}

UItemContainerExtensionBase* UFaerieEquipmentManager::AddExtensionToSlot(const FFaerieSlotTag SlotID,
																		 const TSubclassOf<UItemContainerExtensionBase> ExtensionClass)
{
	if (!ensure(
			IsValid(ExtensionClass) &&
			ExtensionClass != UItemContainerExtensionBase::StaticClass()))
	{
		return nullptr;
	}

	auto&& Slot = FindSlot(SlotID, true);
	if (!IsValid(Slot))
	{
		return nullptr;
	}

	UItemContainerExtensionBase* NewExtension = NewObject<UItemContainerExtensionBase>(Slot, ExtensionClass);
	SET_NEW_IDENTIFIER(NewExtension, TEXTVIEW("NewExt:EquipmentManager"))

	GetOwner()->AddReplicatedSubObject(NewExtension);
	NewExtension->InitializeNetObject(GetOwner());
	Slot->AddExtension(NewExtension);

	return NewExtension;
}

bool UFaerieEquipmentManager::RemoveExtensionFromSlot(const FFaerieSlotTag SlotID, const TSubclassOf<UItemContainerExtensionBase> ExtensionClass)
{
	if (!ensure(
			IsValid(ExtensionClass) &&
			ExtensionClass != UItemContainerExtensionBase::StaticClass()))
	{
		return false;
	}

	auto&& Slot = FindSlot(SlotID, true);
	if (!IsValid(Slot))
	{
		return false;
	}

	UItemContainerExtensionBase* Extension = Slot->GetExtension(ExtensionClass, false);
	if (!IsValid(Extension))
	{
		return false;
	}

	Extension->DeinitializeNetObject(GetOwner());
	GetOwner()->RemoveReplicatedSubObject(Extension);
	Slot->RemoveExtension(Extension);

	return true;
}

void UFaerieEquipmentManager::ForEachContainer(Faerie::TBreakableLoop<UFaerieItemContainerBase*> Iter, const bool Recursive)
{
	if (Recursive)
	{
		for (auto&& Slot : Slots)
		{
			if (Iter(Slot) == Faerie::Stop)
			{
				return;
			}

			if (!Slot->IsFilled()) continue;

			if (UFaerieItemContainerToken::ForEachContainer(Slot->GetItemObject()->MutateCast(), Iter, true) == Faerie::Stop)
			{
				return;
			}
		}
	}
	else
	{
		for (auto&& Element : Slots)
		{
			if (Iter(Element) == Faerie::Stop)
			{
				return;
			}
		}
	}
}

void UFaerieEquipmentManager::ForEachSlot(Faerie::TBreakableLoop<UFaerieEquipmentSlot*> Iter, const bool Recursive)
{
	if (Recursive)
	{
		for (auto&& Slot : Slots)
		{
			if (Iter(Slot) == Faerie::Stop)
			{
				return;
			}

			if (!Slot->IsFilled()) continue;

			if (UFaerieItemContainerToken::ForEachContainer(Slot->GetItemObject()->MutateCast(), Iter, true) == Faerie::Stop)
			{
				return;
			}
		}
	}
	else
	{
		for (auto&& Element : Slots)
		{
			if (Iter(Element) == Faerie::Stop)
			{
				return;
			}
		}
	}
}

TArray<FFaerieItemContainerPath> UFaerieEquipmentManager::GetAllContainerPaths() const
{
	SCOPE_CYCLE_COUNTER(STAT_Equipment_BuildPaths);

	TArray<FFaerieItemContainerPath> OutPaths;
	OutPaths.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		FFaerieItemContainerPath::BuildChildrenPaths(Slot, OutPaths);
	}

	return OutPaths;
}

void UFaerieEquipmentManager::PrintSlotDebugInfo() const
{
#if !UE_BUILD_SHIPPING
	for (auto&& Slot : Slots)
	{
		if (Slot->GetExtensionGroup())
		{
			UE_LOG(LogFaerieEquipment, Log, TEXT("*** Printing Debug Data for: '%s'"), *Slot->Config.SlotID.ToString())
			Slot->GetExtensionGroup()->PrintDebugData();
		}
		else
		{
			UE_LOG(LogFaerieEquipment, Log, TEXT("Slot '%s' has no extension group."), *Slot->Config.SlotID.ToString())
		}
	}
#endif
}
