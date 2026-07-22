// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassEntityQuery.h"
#include "MassObserverProcessor.h"
#include "FaerieItemDataCreationObserver.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieItemDataCreationObserver : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDataCreationObserver();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};