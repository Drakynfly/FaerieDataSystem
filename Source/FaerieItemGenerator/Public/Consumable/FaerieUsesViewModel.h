// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "ViewModels/FaerieItemDataViewModelBase.h"
#include "FaerieUsesViewModel.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieUsesViewModel : public UFaerieItemDataViewModelBase
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
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	int32 MaxUses;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	int32 UsesRemaining;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	bool HasUses = false;
};

namespace Faerie::Generation
{
	USTRUCT()
	struct FUsesViewFragment : public Faerie::Container::FViewModelFragment
	{
		GENERATED_BODY()
	};
}
