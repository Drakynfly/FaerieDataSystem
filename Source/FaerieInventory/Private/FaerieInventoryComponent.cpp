// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryComponent.h"
#include "FaerieItemStorage.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInventoryComponent)

DEFINE_LOG_CATEGORY(LogFaerieInventoryComponent);

UFaerieInventoryComponent::UFaerieInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	ItemStorage = CreateDefaultSubobject<UFaerieItemStorage>(FName{TEXTVIEW("ItemStorage")});
	Extensions = CreateDefaultSubobject<UItemContainerExtensionGroup>(FName{TEXTVIEW("Extensions")});
	Extensions->SetIdentifier();
}

void UFaerieInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemStorage, SharedParams);
}

void UFaerieInventoryComponent::PostInitProperties()
{
	Super::PostInitProperties();

	ItemStorage->GetOnAddressAdded().AddUObject(this, &ThisClass::PostAddressAdded);
	ItemStorage->GetOnAddressUpdated().AddUObject(this, &ThisClass::PostAddressChanged);
	ItemStorage->GetOnAddressRemoved().AddUObject(this, &ThisClass::PreAddressRemoved);
}

void UFaerieInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	AActor* Owner = GetOwner();
	check(IsValid(Owner));

	if (!Owner->HasAuthority()) return;

	if (IsValid(Extensions))
	{
		Extensions->ReplicationFixup();
		ItemStorage->AddExtension(Extensions);
	}

	if (!Owner->IsUsingRegisteredSubObjectList())
	{
		UE_LOG(LogFaerieInventoryComponent, Warning,
			TEXT("Owner of Inventory Component '%s' does not replicate SubObjectList. Inventory will not be replicated correctly!"), *Owner->GetName())
	}
	else
	{
		check(IsValid(ItemStorage));
		check(IsValid(Extensions));

		AddReplicatedSubObject(ItemStorage);
		ItemStorage->InitializeNetObject(Owner);
		AddReplicatedSubObject(Extensions);
		Extensions->InitializeNetObject(Owner);
	}
}

UItemContainerExtensionGroup* UFaerieInventoryComponent::GetExtensionGroup() const
{
	return ItemStorage->GetExtensionGroup();
}

bool UFaerieInventoryComponent::AddExtension(UItemContainerExtensionBase* Extension)
{
	if (ItemStorage->AddExtension(Extension))
	{
		AddReplicatedSubObject(Extension);
		Extension->InitializeNetObject(GetOwner());
		return true;
	}
	return false;
}

bool UFaerieInventoryComponent::RemoveExtension(UItemContainerExtensionBase* Extension)
{
	if (!ensure(IsValid(Extension)))
	{
		return false;
	}

	Extension->DeinitializeNetObject(GetOwner());
	RemoveReplicatedSubObject(Extension);
	return ItemStorage->RemoveExtension(Extension);
}

void UFaerieInventoryComponent::PostAddressAdded(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
#if WITH_EDITOR
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Client Received PostContentAdded"))
	}
	else
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Server Received PostContentAdded"))
	}
#endif
}

void UFaerieInventoryComponent::PostAddressChanged(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
#if WITH_EDITOR
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Client Received PostContentChanged"))
	}
	else
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Server Received PostContentChanged"))
	}
#endif
}

void UFaerieInventoryComponent::PreAddressRemoved(UFaerieItemStorage* Storage, const FFaerieAddress Address)
{
#if WITH_EDITOR
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Client Received PreContentRemoved"))
	}
	else
	{
		UE_LOG(LogFaerieInventoryComponent, Log, TEXT("Server Received PreContentRemoved"))
	}
#endif
}