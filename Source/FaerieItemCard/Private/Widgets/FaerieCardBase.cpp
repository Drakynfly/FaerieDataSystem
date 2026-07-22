// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardBase)

void UFaerieCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshOnDataChange && ItemProxy.IsValid())
	{
		check(!OnDataChangedHandle.IsValid())
		OnDataChangedHandle = ItemProxy.GetOnProxyChangeEvent().AddUObject(this, &ThisClass::OnItemDataChanged);
	}

	if (RefreshOnConstruct)
	{
		Refresh();
	}
}

void UFaerieCardBase::NativeDestruct()
{
	if (OnDataChangedHandle.IsValid())
	{
		if (ItemProxy.IsValid())
		{
			ItemProxy.GetOnProxyChangeEvent().Remove(OnDataChangedHandle);
		}
		OnDataChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void UFaerieCardBase::SetItemData(const FFaerieItemProxy& InItemProxy, const bool bRefresh)
{
	// Unbind from any previous proxy
	if (OnDataChangedHandle.IsValid())
	{
		if (InItemProxy.IsValid())
		{
			ItemProxy.GetOnProxyChangeEvent().Remove(OnDataChangedHandle);
		}
		OnDataChangedHandle.Reset();
	}

	ItemProxy = InItemProxy;

	// Try to bind to new proxy
	if (ItemProxy.IsValid())
	{
		if (IsConstructed() && RefreshOnDataChange)
		{
			OnDataChangedHandle = ItemProxy.GetOnProxyChangeEvent().AddUObject(this, &ThisClass::OnItemDataChanged);
		}
	}

	if (bRefresh)
	{
		Refresh();
	}
}

void UFaerieCardBase::Refresh()
{
	OnCardRefreshed.Broadcast();
	BP_Refresh();
}

void UFaerieCardBase::OnItemDataChanged(const FFaerieItemProxy&, FGameplayTag)
{
	if (ItemProxy.IsValid())
	{
		Refresh();
	}
}
