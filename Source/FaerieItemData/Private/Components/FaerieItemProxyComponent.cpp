﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Components/FaerieItemProxyComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemProxyComponent)

UFaerieItemProxyComponent::UFaerieItemProxyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFaerieItemProxyComponent::SetItemProxy(const FFaerieItemProxy Proxy)
{
	ItemProxy = Proxy;
	OnItemSetNative.Broadcast(this, ItemProxy);
	OnItemSet.Broadcast(this, ItemProxy);
}

void UFaerieItemProxyComponent::ClearItemProxy()
{
	ItemProxy = nullptr;
	OnItemClearNative.Broadcast(this);
	OnItemClear.Broadcast(this);
}