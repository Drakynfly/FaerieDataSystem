// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ViewModels/FaerieItemDataViewModelBase.h"
#include "ViewModels/FaerieViewModelSubsystem.h"

#include "EntityManagerHelpers.h"

FMassEntityHandle UFaerieItemDataViewModelBase::GetItemHandle() const
{
	return ItemProxy.GetItemInstanceOrInvalid().GetMassEntityHandle();
}

void UFaerieItemDataViewModelBase::SetItemProxy(const FFaerieItemProxy& Item)
{
	if (ItemProxy != Item)
	{
		const FFaerieItemProxy OldProxy = ItemProxy;
		ItemProxy = Item;
		UFaerieViewModelSubsystem* ViewModelSubsystem = GetTypedOuter<UFaerieViewModelSubsystem>();
		ViewModelSubsystem->UpdateViewModelAssociation(this, OldProxy);
		OnProxySet(Faerie::ItemData::GetFaerieEntityManagerChecked(Item.ExtractWorld()));
	}
}

void UFaerieItemDataViewModelBase::Return()
{
	if (UFaerieViewModelSubsystem* ViewModelSubsystem = GetTypedOuter<UFaerieViewModelSubsystem>())
	{
		ViewModelSubsystem->ReturnViewModel(this);
	}
}

void UFaerieItemDataViewModelBase::SetItemProxyDirect(FMassEntityManager& EntityManager, const FFaerieItemProxy& Item)
{
	// This is called by the ViewModelSubsystem so we skip updating it, just set and call child impl.
	ItemProxy = Item;
	OnProxySet(EntityManager);
}
