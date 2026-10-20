// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "FaerieItemDataMassReplicators.generated.h"

class UFaerieMassReplicationSubsystem;

/**
 * 
 */
UCLASS()
class UFaerieItemDataCreationReplicator : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataCreationReplicator();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	static void ProcessEntities(FMassExecutionContext& Context, UFaerieMassReplicationSubsystem& ReplicationSubsystem);

	FMassEntityQuery EntityQuery;
};

/**
 *
 */
UCLASS()
class UFaerieItemDataDeletionReplicator : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataDeletionReplicator();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

/**
 *
 */
UCLASS()
class UFaerieItemDataChangeReplicator : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataChangeReplicator();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	static void ProcessEntities(FMassExecutionContext& Context, UFaerieMassReplicationSubsystem& ReplicationSubsystem);

private:
	FMassEntityQuery EventQuery;
};
