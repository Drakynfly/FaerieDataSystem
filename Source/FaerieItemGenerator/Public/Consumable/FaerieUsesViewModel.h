// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassReplication/FaerieViewModelBase.h"
#include "FaerieUsesViewModel.generated.h"

/**
 * 
 */
UCLASS()
class FAERIEITEMGENERATOR_API UFaerieUsesViewModel : public UFaerieViewModelBase
{
	GENERATED_BODY()

public:
	//~ UFaerieViewModelBase
	virtual TNotNull<UScriptStruct*> GetFragmentType() const override;

protected:
	virtual void OnProxySet() override;
	virtual void OnFieldChange(const Faerie::ItemData::FFieldChange& Data) override;
	virtual void CheckForFieldChange(const Faerie::ItemData::FReference& Item, const FConstStructView FragmentView) override;
	//~ UFaerieViewModelBase

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	int32 MaxUses;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	int32 UsesRemaining;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "UsesViewModel")
	bool HasUses = false;
};
