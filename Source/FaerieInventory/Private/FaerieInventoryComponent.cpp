// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieInventoryComponent.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieInventoryLog.h"
#include "FaerieItemStorage.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieInventoryComponent)

UFaerieInventoryComponent::UFaerieInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;

	ItemStorage = CreateDefaultSubobject<UFaerieItemStorage>(FName{TEXTVIEW("ItemStorage")});
}

void UFaerieInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ItemStorage, SharedParams)
}

void UFaerieInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	AActor* Owner = GetOwner();
	check(IsValid(Owner));

	if (!Owner->HasAuthority()) return;

	if (!Owner->IsUsingRegisteredSubObjectList())
	{
		UE_LOGF(LogFaerieInventory, Warning,
			"Owner of Inventory Component '%ls' does not replicate SubObjectList. Inventory will not be replicated correctly!", *Owner->GetName())
	}
	else
	{
		check(IsValid(ItemStorage));

		if (Faerie::Utils::HasLoadFlag(ItemStorage))
		{
			ItemStorage = Faerie::Utils::DuplicateObjectFromDiskForReplication(ItemStorage.Get(), this);
		}
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ItemStorage, this);
		AddReplicatedSubObject(ItemStorage);
		ItemStorage->InitializeContainer(Faerie::ItemData::GetFaerieEntityManagerChecked(GetWorld()));
	}
}