// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "MassReplication/FaerieViewModelBase.h"

void UFaerieViewModelBase::SetItemProxy(const FFaerieItemProxy& Item)
{
	if (ItemProxy != Item)
	{
		const FFaerieItemProxy OldProxy = ItemProxy;
		ItemProxy = Item;
		UFaerieViewModelSubsystem* ViewModelSubsystem = GetTypedOuter<UFaerieViewModelSubsystem>();
		ViewModelSubsystem->UpdateViewModelAssociation(this, OldProxy);
		OnProxySet();
	}
}

void UFaerieViewModelBase::Return()
{
	if (UFaerieViewModelSubsystem* ViewModelSubsystem = GetTypedOuter<UFaerieViewModelSubsystem>())
	{
		ViewModelSubsystem->ReturnViewModel(this);
	}
}

void UFaerieViewModelBase::SetItemProxyDirect(const FFaerieItemProxy& Item)
{
	// This is called by the ViewModelSubsystem so we skip updating it, just set and call child impl.
	ItemProxy = Item;
	OnProxySet();
}
