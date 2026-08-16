// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassReplication/FaerieViewModelBase.h"
#include "FaerieCapacityViewModel.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UFaerieCapacityViewModel : public UFaerieViewModelBase
{
	GENERATED_BODY()

public:
	//~ UFaerieViewModelBase
	virtual TNotNull<UScriptStruct*> GetFragmentType() const override;

protected:
	virtual void OnProxySet(const FMassEntityManager& EntityManager) override;
	virtual void OnFieldChange(const FMassEntityManager& EntityManager, const Faerie::ItemData::FFieldChange& Data) override;
	virtual void CheckForFieldChange(Faerie::TValid<const FFaerieItemInstance&> Item, const FConstStructView FragmentView) override;
	//~ UFaerieViewModelBase

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CapacityViewModel")
	int32 Weight = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CapacityViewModel")
	FIntVector Bounds = FIntVector::ZeroValue;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CapacityViewModel")
	float Efficiency = 1.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "CapacityViewModel")
	bool HasCapacity = false;
};
