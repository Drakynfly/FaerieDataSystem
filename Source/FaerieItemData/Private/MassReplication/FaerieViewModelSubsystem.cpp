// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieViewModelSubsystem.h"
#include "MassReplication/FaerieViewModelBase.h"

#include "FaerieItemOwnerInterface.h"
#include "FaerieItemProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieViewModelSubsystem)

using namespace Faerie;

void UFaerieViewModelSubsystem::Client_PostReplicationChange(const FFaerieItemInstance& Item, const FConstStructView FragmentView)
{
	checkSlow(FragmentView.IsValid())

	if (FFaerieViewModelStorage* Storage = PerTypeViewStorage.Find(FragmentView.GetScriptStruct()))
	{
		for (auto&& InUseView : Storage->InUseViews)
        {
			const FFaerieItemProxy ResolvedOwner(InUseView.Key.ResolveObjectPtr());
        	if (ResolvedOwner.GetItemInstanceOrInvalid() == Item)
        	{
        		InUseView.Value->CheckForFieldChange(Item, FragmentView);
        		return;
        	}
        }
	}
}

UFaerieViewModelBase* UFaerieViewModelSubsystem::GetOrCreateViewModel(const FFaerieItemProxy& Proxy, const TSubclassOf<UFaerieViewModelBase> ViewClass)
{
	if (!IsValid(ViewClass))
	{
		return nullptr;
	}

	const TNotNull<UScriptStruct*> Struct = ViewClass->GetDefaultObject<UFaerieViewModelBase>()->GetFragmentType();
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

	UFaerieViewModelBase* ViewModelToUse;

	if (Storage.UnusedViews.IsEmpty())
	{
		// No blank view models to use, so create a new one.
		ViewModelToUse = NewObject<UFaerieViewModelBase>(this, ViewClass);
		UE_LOG(LogTemp, Verbose, TEXT("Creating new View Model of class '%s'. Total Number: %i"),
			*ViewClass->GetPathName(), Storage.UnusedViews.Num() + Storage.InUseViews.Num() + 1)
	}
	else
	{
		// Get an unused view from the pool.
		ViewModelToUse = Storage.UnusedViews.Pop(EAllowShrinking::No);
		check(ViewModelToUse->ViewModelUsageCount == 0)
	}

	ViewModelToUse->SetItemProxyDirect(Proxy);
	ViewModelToUse->ViewModelUsageCount++;

	if (Proxy.IsValid())
	{
		// If we have a proxy we can associate it with an item here, if not it will fix itself by calling UpdateViewModelAssociation
		Storage.InUseViews.Add(Proxy.GetProxyObject(), ViewModelToUse);
	}

	return ViewModelToUse;
}

void UFaerieViewModelSubsystem::ReturnViewModel(UFaerieViewModelBase* ViewModel)
{
	if (!ensure(IsValid(ViewModel))) return;

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
				UE_LOG(LogTemp, Error, TEXT("Falling back to manual removal, but this is not expected!"))

				// Somehow the item was already destroyed?
				// Manually search for and remove from InUseViews
				for (auto&& It= Storage.InUseViews.CreateIterator(); It; ++It)
				{
					if (It.Value() == ViewModel)
					{
						It.RemoveCurrent();
					}
				}
			}
		}

		ViewModel->SetItemProxyDirect(FFaerieItemProxy());
		Storage.UnusedViews.Add(ViewModel);
	}
}

void UFaerieViewModelSubsystem::HandleFieldChange(const FMassEntityManager& EntityManager, const FFaerieItemInstance& Item, const ItemData::FFieldChange& Data)
{
	if (FFaerieViewModelStorage* Storage = PerTypeViewStorage.Find(Data.StructType))
	{
		for (auto&& InUseView : Storage->InUseViews)
        {
			const FFaerieItemProxy ResolvedOwner(InUseView.Key.ResolveObjectPtr());
        	if (ResolvedOwner.GetItemInstanceOrInvalid() == Item)
        	{
        		InUseView.Value.Get()->OnFieldChange(EntityManager, Data);
        	}
        }
	}
}

void UFaerieViewModelSubsystem::UpdateViewModelAssociation(const TNotNull<UFaerieViewModelBase*> ViewModel, const FFaerieItemProxy& OldProxy)
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
