// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "MVVMViewModelBase.h"
#include "FaerieMassEntityViewModelBase.generated.h"

class UFaerieMassEntityViewModelBase;
struct FMassEntityManager;


#define FAE_API FAERIEITEMDATA_API

namespace Faerie::Container
{
	// Fragment to track view in an entity. Make a child of this to mark queries for a specific view type.
	USTRUCT()
	struct FViewModelFragment : public FMassFragment
	{
		GENERATED_BODY()

		// The active view object for this fragment.
		FWeakObjectPtr ViewObject;
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
	UE_REWRITE bool IsInitialized() const { return EntityHandle.IsValid(); }
	void CreateViewModelEntity(FMassEntityManager& EntityManager, const TNotNull<UScriptStruct*> FragmentType);
	void DestroyViewModelEntity(FMassEntityManager& EntityManager);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|MassEntityViewModel")
	FMassEntityHandle GetEntityHandle() const { return EntityHandle; }

private:
	FMassEntityHandle EntityHandle;
};

#undef FAE_API