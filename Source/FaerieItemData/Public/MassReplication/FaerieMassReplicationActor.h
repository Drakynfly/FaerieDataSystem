// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "FaerieItemInstance.h"
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

	UPROPERTY()
	FFaerieItemStableHandle Item;

	UPROPERTY()
	TArray<FInstancedStruct> Fragments;

	// Flag for when fragments have replicated before the item pointer has. Needs to be fixed up by replication actor tick
	bool AwaitingItemPointer;

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
	void Server_UpdateFragment(const Faerie::ItemData::FReference& Item, TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews);
	void Server_RemoveFragment(const Faerie::ItemData::FReference& Item, TNotNull<const UScriptStruct*> ScriptStruct);
	void Server_RemoveEntity(const Faerie::ItemData::FReference& Item);

	void Client_UpdateEntity(FFaerieMassReplicatedEntity& Entity);
	void Client_RemoveEntity(FFaerieMassReplicatedEntity& Entity);

	void Client_ProcessUpdateData(FFaerieMassReplicatedEntity& Entity);

protected:
	UPROPERTY(Replicated)
	FFaerieMassReplicatedEntities ReplicatedEntities;

	UPROPERTY()
	TObjectPtr<UMassEntitySubsystem> MassEntitySubsystem;

	UPROPERTY()
	TObjectPtr<UFaerieViewModelSubsystem> ViewModelSubsystem;
};
