// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieMassReplicationSubsystem.h"
#include "MassReplication/FaerieMassReplicationActor.h"

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

	if (InWorld.GetNetMode() != NM_Client)
	{
		ReplicationActor = InWorld.SpawnActor<AFaerieMassReplicationActor>();
	}
}

void UFaerieMassReplicationSubsystem::Server_UpdateFragments(const FMassEntityHandle Item,
	const FFaerieNetItemID NetID, const TConstArrayView<FConstStructView> FragmentViews)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_UpdateFragments(Item, NetID, FragmentViews);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveFragments(const FFaerieNetItemID NetID,
	const TConstArrayView<const UScriptStruct*> ScriptStruct)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveFragments(NetID, ScriptStruct);
	}
}

void UFaerieMassReplicationSubsystem::Server_RemoveEntities(const TConstArrayView<FFaerieNetItemID> NetIDs)
{
	if (ensure(IsValid(ReplicationActor)))
	{
		ReplicationActor->Server_RemoveEntities(NetIDs);
	}
}

TOptional<FMassEntityHandle> UFaerieMassReplicationSubsystem::ResolveNetId(const FFaerieNetItemID NetID) const
{
	if (ensure(IsValid(ReplicationActor)))
	{
		return ReplicationActor->ResolveNetId(NetID);
	}
	return NullOpt;
}

void UFaerieMassReplicationSubsystem::ConfirmReplicationActor(AFaerieMassReplicationActor* Actor)
{
	if (ReplicationActor)
	{
		check(ReplicationActor == Actor);
	}

	if (ReplicationActor != Actor)
	{
		ReplicationActor = Actor;
	}
}
