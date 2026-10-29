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

	if (PreparedItemData.Copies > 0)
	{
		PreparedItemData = FFaerieSimpleItemStackSaveData();
		(void)MarkPackageDirty();
	}

	if (StackCopies > 0 &&
		IsValid(SourceAsset))
	{
		FFaerieItemInstancingContext Context;
		Context.CopiesOverride = StackCopies;
		Context.RunningInEditor = true;
		Context.Editor_ItemInstanceOuter = this;
		if (const ItemData::FItemDataBuilder Result = SourceAsset->CreateItemStack(Context);
			Result.IsValid())
		{
			TPair<UFaerieItemAsset*, int32> ItemAsset = Result.GetForEditor();

			FFaerieItemDataDefaults Default;
			Default.ItemAsset = ItemAsset.Get<0>();
			PreparedItemData.ExportData.MassInstances.Add(FInstancedStruct::Make(Default));
			PreparedItemData.Copies = ItemAsset.Get<1>();
			// @todo allow for initializing PreparedItemData.ExportData and .ExtensionData with preconfigured instances.
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
	// Purse existing data from old editor stacks with existing data. We no longer store editor data in the stack.
	if (ItemStack->IsFilled())
	{
		ItemStack = NewObject<UFaerieItemStackContainer>(this);
	}

	if (const UFaerieItemAsset* ItemAsset = Cast<UFaerieItemAsset>(ItemSourceAsset.GetObject()))
	{
		SourceAsset = ItemAsset;
		ItemSourceAsset = nullptr;
		(void)MarkPackageDirty();
	}

	if (HasPreparedItemData())
	{
		if (IsValid(SourceAsset))
		{
			// @todo re-add validation of ExportData at some point
			/*
			const FFaerieItemInstance Instance = FFaerieItemInstance::FromPointer(PreparedItemData.ItemObject);
			if (!Container::ValidateItemData(Instance))
			{
				UE_LOGF(LogFaerieItemMesh, Warning, "Detected out-of-date or invalid stack in: %ls! Regenerating stack.", *GetPathName())
				InitStackFromConfig(false);
			}
			*/
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
	if (HasPreparedItemData())
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
	if (!IsValid(ItemStack))
	{
		return;
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	Utils::ClearLoadFlags(ItemStack);

	AddReplicatedSubObject(ItemStack);
	ItemStack->InitializeContainer(EntityManager);

	Content::SubscribeToContainerEvents(EntityManager, ItemStack, this);

	// @todo setup more complex trigger for initializing runtime instance. some actors might want to wait and only
	// register at a later point.
	if (GetNetMode() < NM_Client)
	{
		if (HasPreparedItemData())
		{
			Container::FLoadParams Params;
			ItemStack->LoadSaveData(EntityManager, PreparedItemData, Params);
		}
	}

	Super::BeginPlay();
}

void AFaerieItemOwningActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ItemData::HasFaerieEntityManagerBeenAssigned(GetWorld()))
	{
		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
		Content::UnsubscribeFromContainerEvents(EntityManager, ItemStack, this);
	}
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
const UFaerieItemAsset* AFaerieItemOwningActorBase::GetItemAsset() const
{
	return SourceAsset;
}
#endif

FMassEntityHandle AFaerieItemOwningActorBase::GetEntityHandle() const
{
	return ItemStack->GetEntityHandle();
}

int32 AFaerieItemOwningActorBase::GetCopies() const
{
	return ItemStack->GetCopies();
}

void AFaerieItemOwningActorBase::OnContainerEventBatch(TNotNull<UFaerieItemContainerBase*> Container,
	TConstArrayView<const Container::FContainerEventPayload*> Events)
{
	RegenerateDataDisplay();
}

bool AFaerieItemOwningActorBase::HasPreparedItemData() const
{
	return !PreparedItemData.ExportData.IsEmpty() && PreparedItemData.Copies > 0;
}

void AFaerieItemOwningActorBase::SetOwnedStack(FMassEntityManager& EntityManager, const FFaerieUnownedItemStack& Stack)
{
	if (ItemStack->IsFilled())
	{
		ItemStack->TakeItemFromSlot(EntityManager, ItemData::EntireStack, Inventory::Tags::RemovalDeletion);
	}
	ItemStack->SetItemInSlot(EntityManager, Stack);
}

void AFaerieItemOwningActorBase::SetOwnedStack(const FFaerieUnownedItemStack& Stack)
{
	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	SetOwnedStack(EntityManager, Stack);
}

#if WITH_EDITOR
void AFaerieItemOwningActorBase::ViewItemObject()
{
	// @todo disabled for now until we can display ExportData
	/*
	if (const UFaerieItem* Item = PreparedItemData.ItemObject)
	{
		FFaerieDataUtilsModule& DataUtilsModule = FModuleManager::GetModuleChecked<FFaerieDataUtilsModule>("FaerieDataUtils");
		DataUtilsModule.AskEditorToOpenObjectEditorWindow(const_cast<UFaerieItem*>(Item));
	}
	*/
}

void AFaerieItemOwningActorBase::RegenerateStack()
{
	InitStackFromConfig(true);
}
#endif