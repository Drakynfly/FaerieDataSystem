// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "ItemContainerExtensionBase.h"
#include "MassProcessor.h"
#include "ItemContainerCountLimit.generated.h"

#define FAE_API FAERIEINVENTORYCONTENT_API

// Limit the total number of items that can be stored in the container
USTRUCT()
struct FAE_API FFaerieItemContainerCountLimit final : public FFaerieItemContainerExtensionBase
{
	GENERATED_BODY()

	//~ FFaerieItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	//~ FFaerieItemContainerExtensionBase

	void PostEvent(const Faerie::Container::FEvent& Event);

	UE_REWRITE int32 GetMaxInstanceCount() const { return MaxInstanceCount; }
	UE_REWRITE int32 GetMaxEntryCount() const { return MaxEntryCount; }

	void SetMaxInstanceCount(int32 Count);
	void SetMaxEntryCount(int32 Count);

	// Retrieve the number of items that this inventory contains.
	int32 GetTotalItemCount() const;

	// Retrieve the number of entries left to be filled.
	int32 GetRemainingEntryCount() const;

	// Retrieve the number of items that this inventory can still contain.
	int32 GetRemainingTotalItemCount() const;

private:
	bool CanContain(const int32 Count) const;

	void UpdateCacheForEntry(FFaerieEntryKey Key, const int32 Delta);
	void RemoveCacheForEntry(FFaerieEntryKey Key);

protected:
	// The maximum number of instances the storage can contain, summed across all entries/stacks.
	UPROPERTY(EditAnywhere, Category = "CountLimit", meta = (ClampMin = 0))
	int32 MaxInstanceCount = 0;

	// The maximum number of entries the storage can contain.
	UPROPERTY(EditAnywhere, Category = "CountLimit", meta = (ClampMin = 0))
	int32 MaxEntryCount = 0;

private:
	TCompactMap<FFaerieEntryKey, int32> EntryAmountCache;

	int32 CurrentTotalItemCopies = 0;

____FAERIE_CONTAINER_DATA_DECL(FFaerieItemContainerCountLimit)
};

UCLASS()
class UFaerieItemContainerCountLimitUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieItemContainerCountLimitUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
};

// Limit the total number of items that can be stored per entry
// @todo this class isn't implemented yet
USTRUCT()
struct FAE_API FFaerieItemContainerPerEntryCountLimit final : public FFaerieItemContainerExtensionBase
{
	GENERATED_BODY()

	//~ FFaerieItemContainerExtensionBase
	virtual void InitializeExtension(TNotNull<const UFaerieItemContainerBase*> Container) override;
	virtual EFaerieExtensionResponse AllowsAddition(TNotNull<const UFaerieItemContainerBase*> Container, const Faerie::Utils::TArrayAdapter<FFaerieItemProxy>& Proxies, FFaerieExtensionAllowsAdditionArgs Args) const override;
	//~ FFaerieItemContainerExtensionBase

	void SetPerEntryMaxInstanceCount(int32 Count);
	void SetPerStackMaxCount(int32 Count);

protected:
	// The maximum number of instances a single entry can contain, summed across all stacks.
	UPROPERTY(EditAnywhere, Category = "CountLimit", meta = (ClampMin = 0))
	int32 PerEntryMaxInstanceCount = 0;

	// The maximum number of copies per stack.
	UPROPERTY(EditAnywhere, Category = "CountLimit", meta = (ClampMin = 0))
	int32 PerStackMaxCount = 0;

____FAERIE_CONTAINER_DATA_DECL(FFaerieItemContainerPerEntryCountLimit)
};

#undef FAE_API