// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "FaerieItemDestructionProcessor.generated.h"

#define FAE_API FAERIEITEMDATA_API

namespace Faerie::ItemData
{
	// This is not a Faerie fragment type, because it needs neither replication nor serialization. It's a runtime only flag.
	USTRUCT()
	struct FMarkedAsGarbage : public FMassSparseTag
	{
		GENERATED_BODY()
	};

	// Marking an item as garbage will cause it to be destroyed after the next Event handling pass.
	FAE_API void MarkItemAsGarbage(FMassEntityManager& EntityManager, FMassEntityHandle Item);
}

/**
 * Destroys all faerie item entities marked for garbage, after all events are broadcast.
 */
UCLASS()
class UFaerieItemDestructionProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemDestructionProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

#undef FAE_API