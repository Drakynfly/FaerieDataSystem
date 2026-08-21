// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieMassFragment.h"
#include "FaerieItemProxy.h"
#include "MVVMViewModelBase.h"

#include "MassReplication/FaerieViewModelSubsystem.h"

#include "FaerieViewModelBase.generated.h"

/**
 * Base class for View Models that inspect fragment data of a Faerie Item.
 */
UCLASS(Abstract)
class FAERIEITEMDATA_API UFaerieViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

	friend class UFaerieViewModelSubsystem;

public:
	// @Todo move to SparseClassStruct???
	virtual TNotNull<UScriptStruct*> GetFragmentType() const
		PURE_VIRTUAL(UFaerieViewModelBase::GetFragmentType, return FFaerieMassFragment::StaticStruct(); )

	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModel")
	void SetItemProxy(const FFaerieItemProxy& Item);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModel")
	const FFaerieItemProxy& GetItemProxy() const { return ItemProxy; }

	// Hand usage of a view model back to the subsystem. This is the same as calling ReturnViewModel directly on the subsystem
	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem")
	void Return();

protected:
	void SetItemProxyDirect(const FFaerieItemProxy& Item);

	// Called by UFaerieViewModelSubsystem
	virtual void OnProxySet(const FMassEntityManager& EntityManager) {}
	virtual void OnFieldChange(const FMassEntityManager& EntityManager, const Faerie::ItemData::FFieldChange& Data) {}
	virtual void CheckForFieldChange(const FFaerieItemInstance& Item, const FConstStructView FragmentView) {}

protected:
	UPROPERTY()
	FFaerieItemProxy ItemProxy;

	uint16 ViewModelUsageCount = 0;
};