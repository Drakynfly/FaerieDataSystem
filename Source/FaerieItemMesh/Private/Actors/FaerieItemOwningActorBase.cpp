// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieItemOwningActorBase.h"
#include "AssetLoadFlagFixer.h"
#include "EntityManagerHelpers.h"
#include "FaerieDataUtilsModule.h"
#include "FaerieItemAsset.h"
#include "FaerieItemInstancingContext.h"
#include "FaerieItemMeshLog.h"
#include "FaerieItemOwnership.h"
#include "FaerieItemStackContainer.h"
#include "ItemContainerEvent.h"
#include "ItemContainerExtensionBase.h"

#include "Modules/ModuleManager.h"
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
	FEditorScriptExecutionGuard ScriptGuard;

	if (ItemStack->IsFilled())
	{
		ItemStack->ClearStackInSlot_Editor();
		(void)MarkPackageDirty();
	}

	if (StackCopies > 0 &&
		IsValid(SourceAsset))
	{
		FFaerieItemInstancingContext Context;
		Context.ItemInstanceOuter = ItemStack;
		Context.CopiesOverride = StackCopies;
		Context.RunningInEditor = true;
		Context.CreateReferencingInstance = true;
		if (const ItemData::FGetInstanceResult Result = SourceAsset->CreateItemStack(Context);
			Result.IsValid())
		{
			// We are knowingly possessing an un-initialized instance to the stack.
			// We will properly initialize the instance in BeginPlay.
			ItemStack->SetItemInSlot_Editor(Result.WithoutInitialization());
			(void)MarkPackageDirty();
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
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SourceAsset))
	{
		InitStackFromConfig(true);
	}
}

void AFaerieItemOwningActorBase::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (const FName PropertyName = PropertyChangedEvent.GetPropertyName();
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SourceAsset))
	{
		InitStackFromConfig(true);
	}
}
#endif

void AFaerieItemOwningActorBase::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (const UFaerieItemAsset* ItemAsset = Cast<UFaerieItemAsset>(ItemSourceAsset.GetObject()))
	{
		SourceAsset = ItemAsset;
		ItemSourceAsset = nullptr;
		(void)MarkPackageDirty();
	}

	if (ItemStack->IsFilled())
	{
		if (IsValid(SourceAsset))
		{
			if (!Container::ValidateItemData(ItemStack->GetItemInstance().GetValue()))
			{
				UE_LOG(LogFaerieItemMesh, Warning, TEXT("Detected out-of-date or invalid stack in: %s! Regenerating stack."), *GetPathName())
				InitStackFromConfig(false);
			}
		}
	}
	else
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

void AFaerieItemOwningActorBase::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, ItemStack, COND_InitialOnly);
}

void AFaerieItemOwningActorBase::BeginPlay()
{
	Utils::ClearLoadFlags(ItemStack);
	Utils::ClearLoadFlags(ItemStack->GetExtensions());

	AddReplicatedSubObject(ItemStack);
	ItemStack->InitializeNetObject(this);
	ItemStack->GetOnContainerEvent().AddUObject(this, &ThisClass::OnItemDataChanged);

	// @todo setup more complex trigger for initializing runtime instance. some actors might want to wait and only
	// register at a later point.
	if (ItemStack->IsFilled())
	{
		auto Instance = ItemStack->GetItemInstance();
		if (Instance.IsSet() && Instance->IsMutable())
		{
			auto EntityManager = ItemData::FRequireEntityManager(this);
			Instance->InitializeMassEntity(EntityManager);

			Container::TakeOwnership(EntityManager, ItemStack, Instance.GetValue());
		}
	}

	Super::BeginPlay();
}

TOptional<FFaerieItemInstance> AFaerieItemOwningActorBase::GetItemInstance() const
{
	return ItemStack->GetItemInstance();
}

int32 AFaerieItemOwningActorBase::GetCopies() const
{
	return ItemStack->GetCopies();
}

IFaerieItemOwnerInterface* AFaerieItemOwningActorBase::GetItemOwner() const
{
	return ItemStack;
}

void AFaerieItemOwningActorBase::SetOwnedStack(const FFaerieUnownedItemStack& Stack)
{
	if (ItemStack->IsFilled())
	{
		ItemStack->TakeItemFromSlot(ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}
	ItemStack->SetItemInSlot(Stack);
}

#if WITH_EDITOR
void AFaerieItemOwningActorBase::ViewItemObject()
{
	const TOptional<FFaerieItemInstance> Instance = GetItemInstance();
	if (!Instance.IsSet()) return;

	if (const UFaerieItem* Item = Instance->GetItemPtr())
	{
		FFaerieDataUtilsModule& DataUtilsModule = FModuleManager::GetModuleChecked<FFaerieDataUtilsModule>("FaerieDataUtils");
		DataUtilsModule.AskEditorToOpenObjectEditorWindow(const_cast<UFaerieItem*>(Item));
	}
}

void AFaerieItemOwningActorBase::RegenerateStack()
{
	InitStackFromConfig(true);
}
#endif

void AFaerieItemOwningActorBase::OnItemDataChanged(const FFaerieItemProxy& Proxy, FGameplayTag Tag)
{
	RegenerateDataDisplay();
}
