// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataFwd.h"
#include "FaerieItemInstance.h"
#include "FaerieItemReplicationTypes.h"
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

	UPROPERTY()
	FFaerieNetItemID NetID;

	// @Todo implement tags

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

namespace Faerie::Container
{
	USTRUCT()
	struct FReplicatedItemMetadata
	{
		GENERATED_BODY()

		// Locally cached entity handle.
		FMassEntityHandle EntityHandle;

		// Is this item awaiting its owning container to replicate before being linked up to its local entity handle.
		bool AwaitingContainerAck = false;
	};
}

template<>
struct TStructOpsTypeTraits<FFaerieMassReplicatedEntities> : public TStructOpsTypeTraitsBase2<FFaerieMassReplicatedEntities>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UCLASS()
class AFaerieMassReplicationActor : public AInfo
{
	GENERATED_BODY()

public:
	AFaerieMassReplicationActor();

	//~ AActor
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
public:
	virtual void Tick(float DeltaSeconds) override;
	//~ AActor

	TOptional<FMassEntityHandle> ResolveNetId(FFaerieNetItemID NetID) const;

	void Server_UpdateFragments(FMassEntityHandle Item, const FFaerieNetItemID NetID, TConstArrayView<FConstStructView> FragmentViews);
	void Server_RemoveFragments(const FFaerieNetItemID NetID, TConstArrayView<const UScriptStruct*> FragmentTypes);
	void Server_RemoveEntities(TConstArrayView<FFaerieNetItemID> NetIDs);

	void Client_AddEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity);

private:
	FMassEntityManager& GetEntityManager();
	void CheckAck(FFaerieNetItemID NetID, Faerie::Container::FReplicatedItemMetadata& Entity);

protected:
	UPROPERTY(Replicated)
	FFaerieMassReplicatedEntities ReplicatedEntities;

	TCompactMap<FFaerieNetItemID, Faerie::Container::FReplicatedItemMetadata> Metadata;

	TSharedPtr<FMassEntityManager> EntityManagerPtr;
};
