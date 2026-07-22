// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "MassSubsystemBase.h"
#include "FaerieItemDataFwd.h"
#include "FaerieItemInstance.h"

#include "StructUtils/StructView.h"

#include "FaerieViewModelSubsystem.generated.h"

class UFaerieViewModelBase;

namespace Faerie::ItemData
{
	struct FFieldChange
	{
		UE_NONCOPYABLE(FFieldChange)

		FFieldChange(const TNotNull<const UScriptStruct*> StructType, const TConstArrayView<FName> Fields UE_LIFETIMEBOUND)
			: StructType(StructType), Fields(Fields) {}

		TNotNull<const UScriptStruct*> StructType;
		TConstArrayView<FName> Fields;
	};
}

USTRUCT()
struct FFaerieViewModelStorage
{
	GENERATED_BODY()

	TMap<FFaerieItemInstance, TWeakObjectPtr<UFaerieViewModelBase>> InUseViews;

	UPROPERTY()
	TArray<TObjectPtr<UFaerieViewModelBase>> UnusedViews;
};

/**
 *
 */
UCLASS()
class FAERIEITEMDATA_API UFaerieViewModelSubsystem : public UMassSubsystemBase
{
	GENERATED_BODY()

	friend class UFaerieViewModelBase;

public:
	// Called by UFaerieMassReplicationSubsystem
	void Client_PostReplicationChange(const Faerie::ItemData::FReference& Item, FConstStructView FragmentView);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem", meta = (DeterminesOutputType = "ViewClass", AutoCreateRefTerm = "Proxy"))
	UFaerieViewModelBase* GetOrCreateViewModel(const FFaerieItemProxy& Proxy, TSubclassOf<UFaerieViewModelBase> ViewClass);

	// Hand usage of a view model back to this subsystem.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ViewModelSubsystem")
	void ReturnViewModel(UFaerieViewModelBase* ViewModel);

	void HandleFieldChange(const Faerie::ItemData::FReference& Item, const Faerie::ItemData::FFieldChange& Data);

protected:
	void UpdateViewModelAssociation(TNotNull<UFaerieViewModelBase*> ViewModel, const FFaerieItemProxy& OldProxy);

private:
	UPROPERTY()
	TMap<TObjectPtr<UScriptStruct>, FFaerieViewModelStorage> PerTypeViewStorage;
};
