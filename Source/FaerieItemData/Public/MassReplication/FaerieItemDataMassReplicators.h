// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "FaerieItemDataMassReplicators.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieItemDataCreationReplicator : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataCreationReplicator();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieItemDataDeletionReplicator : public UMassObserverProcessor
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
class FAERIEITEMDATA_API UFaerieItemDataChangeReplicator : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataChangeReplicator();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
};
