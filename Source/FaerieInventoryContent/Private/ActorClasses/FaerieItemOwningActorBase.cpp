// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ActorClasses/FaerieItemOwningActorBase.h"
#include "FaerieEquipmentSlot.h"
#include "FaerieUtils.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"

AFaerieItemOwningActorBase::AFaerieItemOwningActorBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	ItemStack = CreateDefaultSubobject<UFaerieEquipmentSlot>(TEXT("ItemStack"));
}

void AFaerieItemOwningActorBase::BeginPlay()
{
	Super::BeginPlay();

	Faerie::ClearLoadFlags(ItemStack);
	Faerie::ClearLoadFlags(ItemStack->GetExtensionGroup());

	AddReplicatedSubObject(ItemStack);
	ItemStack->InitializeNetObject(this);

	if (ItemStack->IsFilled())
	{
		// stupid way to reset visuals
		SetSourceProxy(nullptr);
		SetSourceProxy(this);
	}

	ItemStack->GetOnItemChanged().AddUObject(this, &ThisClass::OnItemChanged);
}

void AFaerieItemOwningActorBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ItemStack, COND_InitialOnly);
}

const UFaerieItem* AFaerieItemOwningActorBase::GetItemObject() const
{
	return ItemStack->GetItemObject();
}

int32 AFaerieItemOwningActorBase::GetCopies() const
{
	return ItemStack->GetCopies();
}

TScriptInterface<IFaerieItemOwnerInterface> AFaerieItemOwningActorBase::GetItemOwner() const
{
	return const_cast<ThisClass*>(this);
}

FFaerieItemStack AFaerieItemOwningActorBase::Release(const FFaerieItemStackView Stack)
{
	return ItemStack->Release(Stack);
}

bool AFaerieItemOwningActorBase::Possess(const FFaerieItemStack Stack)
{
	return ItemStack->Possess(Stack);
}

void AFaerieItemOwningActorBase::OnItemChanged(UFaerieEquipmentSlot* FaerieEquipmentSlot)
{
	SetSourceProxy(nullptr);

	if (ItemStack->IsFilled())
	{
		SetSourceProxy(this);
	}
}
