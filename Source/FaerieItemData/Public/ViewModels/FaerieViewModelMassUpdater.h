// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "FaerieViewModelMassUpdater.generated.h"

/**
 * Base class for processors that update view models in response to fragments being updated.
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieViewModelFieldUpdater : public UMassProcessor
{
	GENERATED_BODY()

	friend class UFaerieViewModelSubsystem;

public:
	UFaerieViewModelFieldUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EventQuery;
	FMassEntityQuery ViewQuery;

	UPROPERTY()
	TObjectPtr<const UScriptStruct> ItemDataFragmentType;

	UPROPERTY()
	TObjectPtr<const UScriptStruct> ViewModelFragmentType;
};

/**
 * Tells ViewModels when the entity they view has been destroyed.
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieViewModelEntityDestructionNotifier : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UFaerieViewModelEntityDestructionNotifier();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
