// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "MassEntityManager.h"

#include "GameFramework/Info.h"
#include "StructUtils/InstancedStruct.h"

#include "Iris/ReplicationState/IrisFastArraySerializer.h"

#include "FaerieMassReplicationActor.generated.h"

struct FFaerieMassReplicatedEntities;
class AFaerieMassReplicationActor;

USTRUCT()
struct FFaerieMassReplicatedEntity : public FFastArraySerializerItem
{
	GENERATED_BODY()

	// Locally cached entity handle.
	FMassEntityHandle EntityHandle;

	UPROPERTY()
	TArray<FInstancedStruct> Fragments;

	void PreReplicatedRemove(const FFaerieMassReplicatedEntities& InArraySerializer);
	void PostReplicatedAdd(const FFaerieMassReplicatedEntities& InArraySerializer);
	void PostReplicatedChange(const FFaerieMassReplicatedEntities& InArraySerializer);
};

USTRUCT()
struct FFaerieMassReplicatedEntities : public FIrisFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FFaerieMassReplicatedEntity> Entries;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	TObjectPtr<AFaerieMassReplicationActor> Owner;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FFaerieMassReplicatedEntity, FFaerieMassReplicatedEntities>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FFaerieMassReplicatedEntities> : public TStructOpsTypeTraitsBase2<FFaerieMassReplicatedEntities>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS()
class FAERIEITEMDATA_API AFaerieMassReplicationActor : public AInfo
{
	GENERATED_BODY()

public:
	AFaerieMassReplicationActor();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	void Server_UpdateFragments(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews);
	void Server_RemoveFragments(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<const UScriptStruct*> FragmentTypes);
	void Server_RemoveEntities(const FMassEntityManager& EntityManager, TConstArrayView<FMassEntityHandle> Items);

	void Client_AddEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity);

protected:
	UPROPERTY(Replicated)
	FFaerieMassReplicatedEntities ReplicatedEntities;
};
