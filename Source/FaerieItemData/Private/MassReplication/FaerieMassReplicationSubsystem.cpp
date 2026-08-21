// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationSubsystem.h"
#include "MassReplication/FaerieMassReplicationActor.h"

#include "FaerieItemDataSettings.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "EntityManagerHelpers.h"

#include "Engine/AssetManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassReplicationSubsystem)

using namespace Faerie;

void UFaerieMassReplicationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UMassEntitySubsystem* MassEntitySubsystem = Collection.InitializeDependency<UMassEntitySubsystem>();
	check(MassEntitySubsystem);

	// Setup the Entity Manager singleton for Faerie.
	// @Todo is it worth creating our own separate manager, or using the common one... maybe make this a toggle?
	auto& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
	ItemData::SetFaerieEntityManager(&EntityManager);
}

void UFaerieMassReplicationSubsystem::PostInitialize()
{
	Super::PostInitialize();

	const FSoftObjectPath MassConfigSoftObject = GetDefault<UFaerieItemDataSettings>()->ItemDataMassConfig.ToSoftObjectPath();
	if (MassConfigSoftObject.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid ItemDataMassConfig in ItemDataSettings! Please assign an asset to Project Settings -> Item Data Settings -> ItemDataMassConfig"));
		return;
	}

	// Start to load the item data config asynchronously.
	ItemDataMassConfigStreamHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MassConfigSoftObject,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnItemDataMassConfigLoaded));
}

void UFaerieMassReplicationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ReplicationActor = InWorld.SpawnActor<AFaerieMassReplicationActor>();
}

void UFaerieMassReplicationSubsystem::Deinitialize()
{
	// @todo this can crash things that attempt to access the manager on shutdown, like the auto-save.
	//Faerie::ItemData::SetFaerieEntityManager(nullptr);

	Super::Deinitialize();
}

void UFaerieMassReplicationSubsystem::Server_UpdateFragment(const FFaerieItemInstance& Item,
															const TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews)
{
	if (ensure(IsValid(ReplicationActor) && !FragmentViews.IsEmpty()))
	{
		ReplicationActor->Server_UpdateFragment(Item, FragmentViews);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveFragment(const FFaerieItemInstance& Item,
	const TNotNull<const UScriptStruct*> ScriptStruct)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveFragment(Item, ScriptStruct);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveEntity(const FFaerieItemInstance& Item)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveEntity(Item);
	}
}

const FMassEntityTemplate& UFaerieMassReplicationSubsystem::GetItemDataTemplate()
{
	ForceItemDataTemplateRegistration();
	return ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
}

void UFaerieMassReplicationSubsystem::OnItemDataMassConfigLoaded()
{
	ItemDataMassConfigStreamHandle->ForEachLoadedAsset([&](UObject* LoadedObject)
		{
			if (UMassEntityConfigAsset* ConfigAsset = Cast<UMassEntityConfigAsset>(LoadedObject))
			{
				ItemDataMassConfig = ConfigAsset;
				(void)ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
			}
		});
	ItemDataMassConfigStreamHandle.Reset();
}

void UFaerieMassReplicationSubsystem::ForceItemDataTemplateRegistration()
{
	if (IsValid(ItemDataMassConfig))
	{
		// Config has already been loaded, early out.
		return;
	}

	if (ItemDataMassConfigStreamHandle.IsValid() && ItemDataMassConfigStreamHandle->IsActive())
	{
		// Try flushing the stream handle, which might call OnItemDataMassConfigLoaded.
		ItemDataMassConfigStreamHandle->WaitUntilComplete();
	}

	// If that didn't work, attempt sync loading it directly.
	if (!IsValid(ItemDataMassConfig))
	{
		// Note: Known LoadSync code path; accepted use.
		ItemDataMassConfig = GetDefault<UFaerieItemDataSettings>()->ItemDataMassConfig.LoadSynchronous();
		if (IsValid(ItemDataMassConfig))
		{
			(void)ItemDataMassConfig->GetOrCreateEntityTemplate(*GetWorld());
		}
	}
}