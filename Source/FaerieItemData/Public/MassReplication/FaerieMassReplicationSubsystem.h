// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassSubsystemBase.h"

#include "StructUtils/StructView.h"

#include "FaerieMassReplicationSubsystem.generated.h"

struct FFaerieMassFragment;
class AFaerieMassReplicationActor;

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieMassReplicationSubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()

protected:
	//~ UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	//~ UWorldSubsystem

public:
	void Server_UpdateFragments(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<TConstStructView<FFaerieMassFragment>> FragmentViews);
	void Server_RemoveFragments(const FMassEntityManager& EntityManager, FMassEntityHandle Item, TConstArrayView<const UScriptStruct*> ScriptStruct);
	void Server_RemoveEntities(const FMassEntityManager& EntityManager, TConstArrayView<FMassEntityHandle> Items);

protected:
	UPROPERTY()
	TObjectPtr<AFaerieMassReplicationActor> ReplicationActor;
};
