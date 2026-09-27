// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ViewModels/FaerieItemDataViewModelBase.h"
#include "FaerieCapacityViewModel.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEINVENTORYCONTENT_API UFaerieCapacityViewModel : public UFaerieItemDataViewModelBase
{
	GENERATED_BODY()

public:
	//~ UFaerieItemDataViewModelBase
	virtual TNotNull<UScriptStruct*> GetFragmentType() const override;

protected:
	virtual UScriptStruct* GetViewModelFragmentType() const override;
	virtual void OnProxySet(FMassEntityManager& EntityManager) override;
	//~ UFaerieItemDataViewModelBase

	//~ IFaerieItemDataViewFieldChangeInterface
	virtual void OnFieldChange(const FMassEntityManager& EntityManager, const Faerie::ItemData::FFieldChangePayload& Data) override;
	//~ IFaerieItemDataViewFieldChangeInterface

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


namespace Faerie::Content
{
	USTRUCT()
	struct FCapacityViewFragment : public Container::FViewModelFragment
	{
		GENERATED_BODY()
	};
}