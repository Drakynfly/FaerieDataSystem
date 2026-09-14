// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieContainerDataViewModelBase.h"
#include "FaerieHash.h"
#include "ItemContainerExtensionBase.h"
#include "MVVMViewModelBase.h"
#include "MassProcessor.h"

#include "ContentHashExtension.generated.h"

#define FAE_API FAERIEINVENTORYCONTENT_API

class UFaerieContainerContentHashView;
class UFaerieItemContainerBase;

// @todo this does not trigger RecalcHash on initialization
USTRUCT()
struct FFaerieContainerContentHash : public FFaerieItemContainerData
{
	GENERATED_BODY()

	void RecalcHash(const FMassEntityManager& EntityManager, UFaerieItemContainerBase* InContainer);

	// For clients, this is the last received hash for the serverside equipment state.
	// For servers, this is identical to LocalChecksum.
	UPROPERTY()
	FFaerieHash ServerChecksum;

	// This is the hash of the current local equipment.
	// This is compared against ServerChecksum each time equipment is changed to verify the client has received the
	// current equipment state.
	FFaerieHash LocalChecksum;

____FAERIE_CONTAINER_DATA_DECL(FFaerieContainerContentHash)
};

UCLASS()
class FAE_API UFaerieContainerContentHashView : public UFaerieContainerDataViewModelBase
{
	GENERATED_BODY()

	friend class UFaerieContainerEventLogUpdater;

public:
	//~ UFaerieContainerDataViewModelBase
	virtual void SyncView() override;
	//~ UFaerieContainerDataViewModelBase

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Faerie|ContainerContentHashView")
	bool DoChecksumsMatch() const;

	void CheckAndBroadcast();

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ContainerContentHashView")
	int64 ServerChecksum = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "ContainerContentHashView")
	int64 LocalChecksum = 0;
};

namespace Faerie::Content
{
	USTRUCT()
	struct FContentHashViewFragment : public Container::FViewModelFragment
	{
		GENERATED_BODY()
	};
}

UCLASS()
class UFaerieContainerContentHashUpdater : public UMassProcessor
{
	GENERATED_BODY()

public:
	UFaerieContainerContentHashUpdater();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EventQuery;
	FMassEntityQuery ViewQuery;
};

#undef FAE_API