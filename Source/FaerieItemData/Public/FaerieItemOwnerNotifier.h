// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "FaerieItemOwnerNotifier.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieItemOwnerNotifier : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemOwnerNotifier();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
};
