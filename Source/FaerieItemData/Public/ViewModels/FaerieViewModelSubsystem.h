// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassSubsystemBase.h"

#include "FaerieViewModelSubsystem.generated.h"

struct FFaerieItemProxy;
class UFaerieItemDataViewModelBase;
class UMassProcessor;

USTRUCT()
struct FFaerieViewModelStorage
{
	GENERATED_BODY()

	// The ObjectKey here is the ProxyObject from the FFaerieItemProxy that is set on the ViewModel.
	TMap<FObjectKey, TWeakObjectPtr<UFaerieItemDataViewModelBase>> InUseViews;

	UPROPERTY()
	TArray<TObjectPtr<UFaerieItemDataViewModelBase>> UnusedViews;
};

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieViewModelSubsystem : public UMassTickableSubsystemBase
{
	GENERATED_BODY()

	friend UFaerieItemDataViewModelBase;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem", meta = (DeterminesOutputType = "ViewClass", AutoCreateRefTerm = "Proxy"))
	UFaerieItemDataViewModelBase* GetOrCreateViewModel(TSubclassOf<UFaerieItemDataViewModelBase> ViewClass, const FFaerieItemProxy& Proxy);

	// Hand usage of a view model back to this subsystem.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem")
	void ReturnViewModel(UFaerieItemDataViewModelBase* ViewModel);

protected:
	void UpdateViewModelAssociation(TNotNull<UFaerieItemDataViewModelBase*> ViewModel, const FFaerieItemProxy& OldProxy);

private:
	UPROPERTY()
	TMap<TObjectPtr<UScriptStruct>, FFaerieViewModelStorage> PerTypeViewStorage;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMassProcessor>> ViewModelUpdaters;
};
