// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "FaerieItemDataEventCleanup.generated.h"

/**
 * Destroys item data events after they are handled by other observers.
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieItemDataEventCleanup : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataEventCleanup();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};
