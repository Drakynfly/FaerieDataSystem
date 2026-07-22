// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentManager.h"
#include "EntityManagerHelpers.h"
#include "FaerieEquipmentLog.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

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

UItemContainerExtensionGroup* UFaerieEquipmentManager::VirtualGetExtensionGroup() const
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
			DefaultSlot->GetExtensions()->AddExtension(Element.ExtensionGroup.Get());
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

void UFaerieEquipmentManager::OnDataChangeEvent(const FFaerieItemProxy& Proxy, const FGameplayTag Tag)
{
	if (UFaerieEquipmentSlot* Slot = const_cast<UFaerieEquipmentSlot*>(CastChecked<UFaerieEquipmentSlot>(Proxy.GetProxyObject())))
	{
		BroadcastSlotEvent(Slot, Inventory::Tags::SlotItemMutated);
	}
}

void UFaerieEquipmentManager::BroadcastSlotEvent(const TNotNull<UFaerieEquipmentSlot*> Slot, const FFaerieInventoryTag Event)
{
	OnEquipmentSlotEventNative.Broadcast(Slot, Event);
	OnEquipmentChangedEvent.Broadcast(Slot, Event);
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
	TSharedStruct<FFaerieItemContainerExtensionData> ExtensionData;
	if (!SaveData.ExtensionData.Data.IsEmpty())
	{
		ExtensionData.Initialize(SaveData.ExtensionData);
	}

	for (const FFaerieEquipmentSlotSaveData& PerSlotDatum : SaveData.PerSlotData)
	{
		if (!PerSlotDatum.SlotID.IsValid())
		{
			UE_LOG(LogFaerieEquipment, Error, TEXT("Invalid slot tag found during LoadSaveData!"))
			continue;
		}

		UFaerieEquipmentSlot* EquipmentSlot = FindSlot(PerSlotDatum.SlotID);
		if (!IsValid(EquipmentSlot))
		{
			UE_LOG(LogFaerieEquipment, Warning, TEXT("Save Data contained slot that does not resolve: %s. Implement functionality to convert or discard during load."), *PerSlotDatum.SlotID.ToString())
			continue;
		}

		EquipmentSlot->LoadSlotData(PerSlotDatum, ExtensionData);
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

		NewSlot->OnItemChangedNative.AddUObject(this, &ThisClass::OnDataChangeEvent);

		NewSlot->GetExtensions()->SetParentGroup(ExtensionGroup);

		BroadcastSlotEvent(NewSlot, Equipment::Tags::SlotCreated);

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
		BroadcastSlotEvent(Slot, Equipment::Tags::SlotDeleted);

		Slot->GetExtensions()->ClearParentGroup();

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

bool UFaerieEquipmentManager::TrySwapSlots(UFaerieEquipmentSlot* SlotA, UFaerieEquipmentSlot* SlotB)
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
	SlotB->SetStoredItem_Impl(FFaerieItemDataView(ContentA.Instance, ContentA.Copies, nullptr));
	SlotA->SetStoredItem_Impl(FFaerieItemDataView(ContentB.Instance, ContentB.Copies, nullptr));

	return true;
}

const UFaerieEquipmentSlot* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive) const
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
		ItemData::FRequireEntityManager EntityManager(this);
		for (auto&& Slot : Slots)
		{
			if (!IsValid(Slot)) continue;
			if (auto&& ChildSlot = Slot->FindSlot(EntityManager, SlotID, true))
			{
				return ChildSlot;
			}
		}
	}

	return nullptr;
}

UFaerieEquipmentSlot* UFaerieEquipmentManager::FindSlot(const FFaerieSlotTag SlotID, const bool Recursive)
{
	return const_cast<UFaerieEquipmentSlot*>(const_cast<const UFaerieEquipmentManager*>(this)->FindSlot(SlotID, Recursive));
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
	Slot->GetExtensions()->AddExtension(NewExtension);

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

	UItemContainerExtensionBase* Extension = Extensions::Get(Slot->GetExtensions(), ExtensionClass, false);
	if (!IsValid(Extension))
	{
		return false;
	}

	Extension->DeinitializeNetObject(GetOwner());
	GetOwner()->RemoveReplicatedSubObject(Extension);
	Slot->GetExtensions()->RemoveExtension(Extension);

	return true;
}

TArray<FFaerieItemContainerPath> UFaerieEquipmentManager::GetAllContainerPaths(UObject* WorldContextObj) const
{
	SCOPE_CYCLE_COUNTER(STAT_Equipment_BuildPaths);

	ItemData::FRequireEntityManager EntityManager(WorldContextObj);
	TArray<FFaerieItemContainerPath> OutPaths;
	OutPaths.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		if (Slot->IsFilled())
		{
			FFaerieItemContainerPath::BuildChildrenPaths(EntityManager, Slot->GetItemInstance().GetValue(), Slot, OutPaths);
		}
	}

	return OutPaths;
}

void UFaerieEquipmentManager::PrintSlotDebugInfo() const
{
#if !UE_BUILD_SHIPPING
	for (auto&& Slot : Slots)
	{
		if (Slot->GetExtensions())
		{
			UE_LOG(LogFaerieEquipment, Log, TEXT("*** Printing Debug Data for: '%s'"), *Slot->Config.SlotID.ToString())
			Slot->GetExtensions()->PrintDebugData();
		}
		else
		{
			UE_LOG(LogFaerieEquipment, Log, TEXT("Slot '%s' has no extension group."), *Slot->Config.SlotID.ToString())
		}
	}
#endif
}
