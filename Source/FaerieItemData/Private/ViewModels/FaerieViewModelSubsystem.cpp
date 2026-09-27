// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "EntityManagerHelpers.h"

#include "ViewModels/FaerieViewModelSubsystem.h"
#include "ViewModels/FaerieItemDataViewModelBase.h"

#include "FaerieItemDataLog.h"

#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"
#include "MassEntitySubsystem.h"
#include "MassExecutor.h"
#include "MassProcessingContext.h"

#include "ViewModels/FaerieViewModelMassUpdater.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieViewModelSubsystem)

using namespace Faerie;

void UFaerieViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UMassEntitySubsystem>();

	OverrideSubsystemTraits<ThisClass>(Collection);

	TArray<UClass*> ViewModelClasses;
	GetDerivedClasses(UFaerieItemDataViewModelBase::StaticClass(), ViewModelClasses);

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());

	for (const UClass* ViewModelClass : ViewModelClasses)
	{
		if (ViewModelClass->HasAnyClassFlags(EClassFlags::CLASS_Abstract))
		{
			continue;
		}

		const UFaerieItemDataViewModelBase* ViewModelCDO = ViewModelClass->GetDefaultObject<UFaerieItemDataViewModelBase>();
		const UScriptStruct* FragmentType = ViewModelCDO->GetFragmentType();
		const UScriptStruct* ViewModelType = ViewModelCDO->GetViewModelFragmentType();

		if (FragmentType && ViewModelType)
		{
			UFaerieViewModelFieldUpdater* Updater = NewObject<UFaerieViewModelFieldUpdater>(this);
			Updater->ItemDataFragmentType = FragmentType;
			Updater->ViewModelFragmentType = ViewModelType;
			Updater->CallInitialize(this, EntityManager.AsShared());
			ViewModelUpdaters.Add(Updater);
		}
	}
}

void UFaerieViewModelSubsystem::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	FMassProcessingContext ProcessingContext(EntityManager, DeltaTime);
	UE::Mass::Executor::RunProcessorsView(ViewModelUpdaters, ProcessingContext);
}

TStatId UFaerieViewModelSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFaerieViewModelSubsystem, STATGROUP_Tickables);
}

UFaerieItemDataViewModelBase* UFaerieViewModelSubsystem::GetOrCreateViewModel(const TSubclassOf<UFaerieItemDataViewModelBase> ViewClass, const FFaerieItemProxy& Proxy)
{
	if (!IsValid(ViewClass))
	{
		return nullptr;
	}

	const TNotNull<UScriptStruct*> Struct = ViewClass->GetDefaultObject<UFaerieItemDataViewModelBase>()->GetFragmentType();
	FFaerieViewModelStorage& Storage = PerTypeViewStorage.FindOrAdd(Struct);

	if (Proxy.IsValid())
	{
		// Re-use existing view if there is already one set to this item.
        if (auto&& ExistingView = Storage.InUseViews.Find(Proxy.GetProxyObject());
        	ExistingView && ExistingView->IsValid())
        {
        	ExistingView->Get()->ViewModelUsageCount++;
        	return ExistingView->Get();
        }
	}

	// A null Item is allowed because some UI create the ViewModel and them populate it with an item dynamically.

	UFaerieItemDataViewModelBase* ViewModelToUse;

	if (Storage.UnusedViews.IsEmpty())
	{
		// No blank view models to use, so create a new one.
		ViewModelToUse = NewObject<UFaerieItemDataViewModelBase>(this, ViewClass);
		UE_LOGF(LogFaerieItemData, Verbose, "Creating new View Model of class '%ls'. Total Number: %i",
			*ViewClass->GetPathName(), Storage.UnusedViews.Num() + Storage.InUseViews.Num() + 1)
	}
	else
	{
		// Get an unused view from the pool.
		ViewModelToUse = Storage.UnusedViews.Pop(EAllowShrinking::No);
		check(ViewModelToUse->ViewModelUsageCount == 0)
	}

	FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
	ViewModelToUse->SetItemProxyDirect(EntityManager, Proxy);
	ViewModelToUse->ViewModelUsageCount++;

	if (Proxy.IsValid())
	{
		// If we have a proxy we can associate it with an item here, if not it will fix itself by calling UpdateViewModelAssociation
		Storage.InUseViews.Add(Proxy.GetProxyObject(), ViewModelToUse);
	}

	return ViewModelToUse;
}

void UFaerieViewModelSubsystem::ReturnViewModel(UFaerieItemDataViewModelBase* ViewModel)
{
	if (!IsValid(ViewModel)) return;

	const TNotNull<UScriptStruct*> Struct = ViewModel->GetFragmentType();
	FFaerieViewModelStorage& Storage = PerTypeViewStorage.FindOrAdd(Struct);

	check(ViewModel->ViewModelUsageCount > 0)
	if (--ViewModel->ViewModelUsageCount == 0)
	{
		if (const UObject* ProxyObject = ViewModel->GetItemProxy().GetProxyObject())
		{
			const bool Removed = !!Storage.InUseViews.Remove(ProxyObject);

			if (!Removed)
			{
				UE_LOGF(LogFaerieItemData, Error, "Falling back to manual removal, but this is not expected!")

				// Somehow the item was already destroyed?
				// Manually search for and remove from InUseViews
				for (auto&& It = Storage.InUseViews.CreateIterator(); It; ++It)
				{
					if (It.Value() == ViewModel)
					{
						It.RemoveCurrent();
					}
				}
			}
		}

		FMassEntityManager& EntityManager = ItemData::GetFaerieEntityManagerChecked(GetWorld());
		ViewModel->SetItemProxyDirect(EntityManager, FFaerieItemProxy());
		Storage.UnusedViews.Add(ViewModel);
	}
}

void UFaerieViewModelSubsystem::UpdateViewModelAssociation(const TNotNull<UFaerieItemDataViewModelBase*> ViewModel, const FFaerieItemProxy& OldProxy)
{
	if (FFaerieViewModelStorage* Storage = PerTypeViewStorage.Find(ViewModel->GetFragmentType()))
	{
		if (OldProxy.IsValid())
		{
			Storage->InUseViews.Remove(OldProxy.GetProxyObject());
		}

		if (const FFaerieItemProxy& NewProxy = ViewModel->GetItemProxy();
			NewProxy.IsValid())
		{
			Storage->InUseViews.Add(NewProxy.GetProxyObject(), ViewModel);
		}
	}
}
