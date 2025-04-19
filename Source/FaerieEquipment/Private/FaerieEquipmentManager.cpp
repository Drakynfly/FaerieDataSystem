// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieEquipmentManager.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieItemStorage.h"
#include "FaerieUtils.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieEquipmentManager)

DEFINE_LOG_CATEGORY(LogEquipmentManager)

UFaerieEquipmentManager::UFaerieEquipmentManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	ExtensionGroup = CreateDefaultSubobject<UItemContainerExtensionGroup>("ExtensionGroup");
	ExtensionGroup->SetIdentifier();
}

void UFaerieEquipmentManager::PostInitProperties()
{
	Super::PostInitProperties();

	for (auto&& DefaultSlot : InstanceDefaultSlots)
	{
		if (IsValid(DefaultSlot.ExtensionGroup))
		{
			DefaultSlot.ExtensionGroup->SetIdentifier();
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

	AddDefaultSlots();
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

	for (auto&& Element : InstanceDefaultSlots)
	{
		auto&& DefaultSlot = AddSlot(Element.SlotConfig);
		if (!IsValid(DefaultSlot))
		{
			continue;
		}

		if (IsValid(Element.ExtensionGroup))
		{
			// The default ExtensionGroups are "Assets" in that they are default instanced baked into the objects, and
			// need to be fixed before they can replicate.
			DefaultSlot->AddExtension(Faerie::DuplicateObjectFromDiskForReplication(Element.ExtensionGroup, DefaultSlot));
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
		UE_LOG(LogTemp, Warning,
			TEXT("Owner of Equipment Manager '%s' does not replicate SubObjectList. Component will not be replicated correctly!"), *Owner->GetName())
	}
	else
	{
		AddReplicatedSubObject(ExtensionGroup);
		ExtensionGroup->InitializeNetObject(Owner);

		for (auto&& Slot : Slots)
		{
			if (IsValid(Slot))
			{
				AddReplicatedSubObject(Slot);
				Slot->InitializeNetObject(Owner);
			}
		}

		// Make slots replicate once
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
	}
}

void UFaerieEquipmentManager::OnSlotItemChanged(UFaerieEquipmentSlot* Slot, const bool TokenEdit)
{
	const EFaerieEquipmentSlotChangeType Type = TokenEdit ? EFaerieEquipmentSlotChangeType::TokenEdit : EFaerieEquipmentSlotChangeType::ItemChange;
	OnEquipmentChangedEventNative.Broadcast(Slot, Type);
	OnEquipmentChangedEvent.Broadcast(Slot, Type);
}

FFaerieContainerSaveData UFaerieEquipmentManager::MakeSaveData() const
{
	FFaerieEquipmentSaveData SlotSaveData;
	SlotSaveData.PerSlotData.Reserve(Slots.Num());
	for (auto&& Slot : Slots)
	{
		SlotSaveData.PerSlotData.Add(Slot->MakeSaveData());
	}

	FFaerieContainerSaveData SaveData;
	SaveData.ItemData = FInstancedStruct::Make(SlotSaveData);
	return SaveData;
}

void UFaerieEquipmentManager::LoadSaveData(const FFaerieContainerSaveData& SaveData)
{
	Slots.Reset();

	const FFaerieEquipmentSaveData& EquipmentSaveData = SaveData.ItemData.Get<FFaerieEquipmentSaveData>();
	for (const FFaerieContainerSaveData& SlotSaveData : EquipmentSaveData.PerSlotData)
	{
		if (UFaerieEquipmentSlot* NewSlot = NewObject<UFaerieEquipmentSlot>(this))
		{
			NewSlot->LoadSaveData(SlotSaveData);
			Slots.Add(NewSlot);
			NewSlot->OnItemChangedNative.AddUObject(this, &ThisClass::OnSlotItemChanged, false);
			NewSlot->OnItemDataChangedNative.AddUObject(this, &ThisClass::OnSlotItemChanged, true);
			NewSlot->AddExtension(ExtensionGroup);
		}
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this);

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
		NewSlot->Config = Config;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		Slots.Add(NewSlot);
		AddReplicatedSubObject(NewSlot);
		NewSlot->InitializeNetObject(GetOwner());

		NewSlot->OnItemChangedNative.AddUObject(this, &ThisClass::OnSlotItemChanged, false);
		NewSlot->OnItemDataChangedNative.AddUObject(this, &ThisClass::OnSlotItemChanged, true);

		NewSlot->AddExtension(ExtensionGroup);

		OnEquipmentSlotAddedNative.Broadcast(NewSlot);
		OnEquipmentSlotAdded.Broadcast(NewSlot);

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
		OnPreEquipmentSlotRemovedNative.Broadcast(Slot);
		OnPreEquipmentSlotRemoved.Broadcast(Slot);

		Slot->RemoveExtension(ExtensionGroup);

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Slots, this)
		RemoveReplicatedSubObject(Slot);

		Slot->OnItemChangedNative.RemoveAll(this);
		Slot->OnItemDataChangedNative.RemoveAll(this);

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
		AddReplicatedSubObject(Extension);
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

	RemoveReplicatedSubObject(Extension);
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
	NewExtension->SetIdentifier();

	AddReplicatedSubObject(NewExtension);
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

	RemoveReplicatedSubObject(Extension);
	Slot->RemoveExtension(Extension);

	return true;
}