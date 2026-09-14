// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MVVMViewModelBase.h"

#include "Fragments/ContainerMetadataFragment.h"

#include "FaerieContainerDataViewModelBase.generated.h"

#define FAE_API FAERIEINVENTORY_API

class UFaerieMassEntityViewModelBase;
struct FMassEntityManager;

namespace Faerie::Container
{
	// Fragment to track view in an entity. Make a child of this to mark queries for a specific view type.
	USTRUCT()
	struct FViewModelFragment : public FMassFragment
	{
		GENERATED_BODY()

		// The active view object for this fragment.
		TWeakObjectPtr<UFaerieMassEntityViewModelBase> ViewObject;
	};
}


/**
 * Base class for View Model objects that have a Mass Entity bound to them.
 */
UCLASS(Abstract)
class FAE_API UFaerieMassEntityViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

protected:
	void InitEntity(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|MassEntityViewModel")
	FMassEntityHandle GetEntityHandle() const { return EntityHandle; }

private:
	FMassEntityHandle EntityHandle;
};

/**
 * Base class for View Model objects that reflect Faerie Container data.
 */
UCLASS(Abstract)
class FAE_API UFaerieContainerDataViewModelBase : public UFaerieMassEntityViewModelBase
{
	GENERATED_BODY()

public:
	// Call to set up this view. The entity will be created, and then SyncView will be called.
	void InitializeView(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType, const TPair<FWeakObjectPtr, struct FFaerieItemContainerExtensions*>& ExtensionPtrTemp);

	virtual void SyncView() {}

	UObject* GetContainerObject() const { return ContainerExtensionPtr.Key.Get(); }

protected:
	// The container that we view the log for.
	TPair<FWeakObjectPtr, struct FFaerieItemContainerExtensions*> ContainerExtensionPtr;
};

#undef FAE_API
