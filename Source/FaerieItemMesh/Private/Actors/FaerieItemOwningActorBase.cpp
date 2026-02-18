// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieItemOwningActorBase.h"
#include "AssetLoadFlagFixer.h"
#include "FaerieItemSource.h"
#include "FaerieItemStackContainer.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemOwningActorBase)

using namespace Faerie;

AFaerieItemOwningActorBase::AFaerieItemOwningActorBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bReplicateUsingRegisteredSubObjectList = true;

	ItemStack = CreateDefaultSubobject<UFaerieItemStackContainer>(TEXT("ItemStack"));
}

#if WITH_EDITOR
void AFaerieItemOwningActorBase::InitStackFromConfig(const bool RegenerateDisplay)
{
	if (ItemStack->IsFilled())
	{
		ItemStack->TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}

	if (StackCopies > 0 &&
		IsValid(ItemSourceAsset.GetObject()))
	{
		FFaerieItemInstancingContext Context;
		Context.CopiesOverride = StackCopies;
		if (auto NewStack = ItemSourceAsset->CreateItemStack(&Context);
			NewStack.IsSet())
		{
			ItemStack->Possess(MoveTemp(NewStack.GetValue()));
		}
	}

	if (RegenerateDisplay)
	{
		RegenerateDataDisplay();
	}
}

void AFaerieItemOwningActorBase::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, StackCopies) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, ItemSourceAsset))
	{
		FEditorScriptExecutionGuard ScriptGuard;
		InitStackFromConfig(true);
	}
}

void AFaerieItemOwningActorBase::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, ItemSourceAsset))
	{
		FEditorScriptExecutionGuard ScriptGuard;
		InitStackFromConfig(true);
	}
}
#endif

void AFaerieItemOwningActorBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!ItemStack->IsFilled())
	{
		InitStackFromConfig(false);
	}
#endif
}

void AFaerieItemOwningActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (ItemStack->IsFilled())
	{
		if (RegenerateDisplayOnConstruction)
		{
			RegenerateDataDisplay();
		}
	}
	else
	{
		InitStackFromConfig(true);
	}
#endif
}

void AFaerieItemOwningActorBase::BeginPlay()
{
	Utils::ClearLoadFlags(ItemStack);
	Utils::ClearLoadFlags(ItemStack->GetExtensionGroup());

	AddReplicatedSubObject(ItemStack);
	ItemStack->InitializeNetObject(this);
	ItemStack->GetOnContainerEvent().AddUObject(this, &ThisClass::OnItemChanged);

	Super::BeginPlay();
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

FFaerieItemStack AFaerieItemOwningActorBase::Release(const int32 Copies) const
{
	// RegenerateDataDisplay will be triggered by the Slot broadcasting to OnItemChanged
	return ItemStack->Release(Copies);
}

FFaerieItemStack AFaerieItemOwningActorBase::Release(const FFaerieItemStackView Stack)
{
	if (Stack.Item == ItemStack->GetItemObject())
	{
		// RegenerateDataDisplay will be triggered by the Slot broadcasting to OnItemChanged
		return ItemStack->Release(Stack.Copies);
	}
	return FFaerieItemStack();
}

bool AFaerieItemOwningActorBase::Possess(const FFaerieItemStack Stack)
{
	// RegenerateDataDisplay will be triggered by the Slot broadcasting to OnItemChanged
	return ItemStack->Possess(Stack);
}

void AFaerieItemOwningActorBase::SetOwnedStack(const FFaerieItemStack& Stack)
{
	if (ItemStack->IsFilled())
	{
		ItemStack->TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}
	ItemStack->Possess(Stack);
}

void AFaerieItemOwningActorBase::OnItemChanged(UFaerieItemStackContainer* FaerieEquipmentSlot, FFaerieInventoryTag Event)
{
	RegenerateDataDisplay();
}
