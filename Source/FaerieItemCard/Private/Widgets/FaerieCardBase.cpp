// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardBase.h"
#include "ViewModels/FaerieProxyViewModel.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardBase)

void UFaerieCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshOnConstruct)
	{
		Refresh();
	}
}

void UFaerieCardBase::SetItemData(const FFaerieItemProxy& InItemProxy, const bool bRefresh)
{
	ItemProxy = InItemProxy;

	// If the proxy is a view model, we can update ourself with stack count changes.
	if (UFaerieProxyViewModelBase* ViewModel = Cast<UFaerieProxyViewModelBase>(const_cast<UObject*>(InItemProxy.GetProxyObject())))
	{
		if (UMVVMView* View = GetExtension<UMVVMView>())
		{
			View->SetViewModelByClass(ViewModel);
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