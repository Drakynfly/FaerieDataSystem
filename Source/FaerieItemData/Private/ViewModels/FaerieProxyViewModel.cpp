// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "ViewModels/FaerieProxyViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieProxyViewModel)

void UFaerieProxyViewModelBase::CheckItemCount(const int32 NewCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(StackCount, NewCount);
}
