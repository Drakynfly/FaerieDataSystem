// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "MassObserverProcessor.h"
#include "MassProcessor.h"

#include "Misc/DateTime.h"
#include "ModificationDateFragment.generated.h"

/*
 * Keeps track of the last time this item was modified. Allows, for example, sorting items by recently touched.
 */
USTRUCT()
struct FFaerieItemModificationDate : public FFaerieMassFragment
{
	GENERATED_BODY()

	FFaerieItemModificationDate() = default;
	FFaerieItemModificationDate(const FDateTime& LastModified)
	  : LastModified(LastModified) {}

	UPROPERTY()
	FDateTime LastModified = FDateTime();

____FAERIE_FRAGMENT_DECL(FFaerieItemModificationDate)
};

UCLASS()
class UFaerieItemModificationDataInitializer : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemModificationDataInitializer();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

UCLASS()
class UFaerieItemModificationDateUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemModificationDateUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
};
