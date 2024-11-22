﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardBase)

void UFaerieCardBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshOnConstruct)
	{
		Refresh();
	}
}

void UFaerieCardBase::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	const ThisClass* This = CastChecked<ThisClass>(InThis);
	FWeakObjectPtr WeakPtr = This->ItemProxy.GetObject();
	Collector.AddReferencedObject(WeakPtr);
}

void UFaerieCardBase::SetItemData(const FFaerieItemProxy InItemProxy, const bool bRefresh)
{
	ItemProxy = InItemProxy;
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