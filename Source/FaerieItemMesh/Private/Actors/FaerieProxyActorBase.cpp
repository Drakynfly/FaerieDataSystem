// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieProxyActorBase.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieProxyActorBase)

AFaerieProxyActorBase::AFaerieProxyActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

const UFaerieItem* AFaerieProxyActorBase::GetItemObject() const
{
	return DataSource.GetItemObject();
}

int32 AFaerieProxyActorBase::GetCopies() const
{
	return DataSource.GetCopies();
}

TScriptInterface<IFaerieItemOwnerInterface> AFaerieProxyActorBase::GetItemOwner() const
{
	return DataSource.GetOwner();
}

FFaerieItemStack AFaerieProxyActorBase::Release(const int32 Copies) const
{
	return DataSource.Release(Copies);
}

void AFaerieProxyActorBase::SetSourceProxy(const FFaerieItemProxy Source)
{
	if (Source != DataSource)
	{
		DataSource = Source;
		RegenerateDataDisplay();
	}
}
