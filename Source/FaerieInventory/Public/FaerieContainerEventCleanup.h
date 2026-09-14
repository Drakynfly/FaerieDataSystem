// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "FaerieContainerEventCleanup.generated.h"

/**
 * Destroys events after they are handled by other observers.
 */
UCLASS()
class FAERIEINVENTORY_API UFaerieContainerEventCleanup : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerEventCleanup();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
