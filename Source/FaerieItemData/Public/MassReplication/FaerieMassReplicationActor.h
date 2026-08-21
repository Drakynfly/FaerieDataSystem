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
class UFaerieViewModelSubsystem;
class UMassEntitySubsystem;

USTRUCT()
struct FFaerieMassReplicatedEntity : public FFastArraySerializerItem
{
	GENERATED_BODY()

	// Flag to detect when ItemPointer has replicated to the client, but hasn't been pushed into mass yet.
	bool HasImportedItemPointer = false;

	// Locally cached entity handle.
	FMassEntityHandle EntityHandle;

	// The ItemAsset for this item. May not always be valid.
	UPROPERTY()
	TObjectPtr<UFaerieItem> ItemPointer;

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

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaSeconds) override;

public:
	void Server_UpdateFragment(const FFaerieItemInstance& Item, TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews);
	void Server_RemoveFragment(const FFaerieItemInstance& Item, TNotNull<const UScriptStruct*> ScriptStruct);
	void Server_RemoveEntity(const FFaerieItemInstance& Item);

	void Client_AddEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_RemoveEntity(const FFaerieMassReplicatedEntity& Entity);

	void Client_CheckItemPointer(FFaerieMassReplicatedEntity& Entity);

protected:
	UPROPERTY(Replicated)
	FFaerieMassReplicatedEntities ReplicatedEntities;

	UPROPERTY()
	TObjectPtr<UFaerieViewModelSubsystem> ViewModelSubsystem;
};
