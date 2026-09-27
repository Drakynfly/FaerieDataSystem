// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationSubsystem.h"
#include "MassReplication/FaerieMassReplicationActor.h"

#include "MassEntityConfigAsset.h"

#include "Engine/AssetManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieMassReplicationSubsystem)

void UFaerieMassReplicationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OverrideSubsystemTraits<ThisClass>(Collection);
}

void UFaerieMassReplicationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	ReplicationActor = InWorld.SpawnActor<AFaerieMassReplicationActor>();
}

void UFaerieMassReplicationSubsystem::Server_UpdateFragments(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
															const TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews)
{
	if (ensure(IsValid(ReplicationActor) && !FragmentViews.IsEmpty()))
	{
		ReplicationActor->Server_UpdateFragments(EntityManager, Item, FragmentViews);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveFragments(const FMassEntityManager& EntityManager, const FMassEntityHandle Item,
	const TConstArrayView<const UScriptStruct*> ScriptStruct)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveFragments(EntityManager, Item, ScriptStruct);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveEntities(const FMassEntityManager& EntityManager, const TConstArrayView<FMassEntityHandle> Items)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveEntities(EntityManager, Items);
	}
}