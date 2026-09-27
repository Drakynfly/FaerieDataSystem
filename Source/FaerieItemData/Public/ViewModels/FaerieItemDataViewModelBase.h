// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataViewFieldChangeInterface.h"
#include "FaerieMassFragment.h"
#include "FaerieItemProxy.h"
#include "FaerieMassEntityViewModelBase.h"
#include "StructUtils/StructView.h"

#include "FaerieItemDataViewModelBase.generated.h"

/**
 * Base class for View Models that inspect fragment data of a Faerie Item.
 */
UCLASS(Abstract)
class FAERIEITEMDATA_API UFaerieItemDataViewModelBase : public UFaerieMassEntityViewModelBase, public IFaerieItemDataViewFieldChangeInterface
{
	GENERATED_BODY()

	friend class UFaerieViewModelFieldUpdater;
	friend class UFaerieViewModelSubsystem;

public:
	//~ IFaerieItemDataViewFieldChangeInterface
	virtual FMassEntityHandle GetItemHandle() const final override;
	//~ IFaerieItemDataViewFieldChangeInterface

public:
	virtual TNotNull<UScriptStruct*> GetFragmentType() const
		PURE_VIRTUAL(UFaerieViewModelBase::GetFragmentType, return FFaerieMassFragment::StaticStruct(); )

protected:
	// Implement to generate a View Model updater bound to this class.
	virtual UScriptStruct* GetViewModelFragmentType() const { return nullptr; }

	// Called by UFaerieViewModelSubsystem
	virtual void OnProxySet(FMassEntityManager& EntityManager) {}

	void SetItemProxyDirect(FMassEntityManager& EntityManager, const FFaerieItemProxy& Item);

public:
	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModel")
	void SetItemProxy(const FFaerieItemProxy& Item);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModel")
	const FFaerieItemProxy& GetItemProxy() const { return ItemProxy; }

	// Hand usage of a view model back to the subsystem. This is the same as calling ReturnViewModel directly on the subsystem
	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem")
	void Return();

protected:
	UPROPERTY()
	FFaerieItemProxy ItemProxy;

	uint16 ViewModelUsageCount = 0;
};