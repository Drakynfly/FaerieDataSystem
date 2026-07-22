// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieProxyActorBase.h"
#include "FaerieItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieProxyActorBase)

AFaerieProxyActorBase::AFaerieProxyActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

TOptional<FFaerieItemInstance> AFaerieProxyActorBase::GetItemInstance() const
{
	if (!DataSource.IsValid())
	{
		return NullOpt;
	}

	return DataSource->GetItemInstance();
}

int32 AFaerieProxyActorBase::GetCopies() const
{
	if (!DataSource.IsValid())
	{
		return -1;
	}

	return DataSource->GetCopies();
}

IFaerieItemOwnerInterface* AFaerieProxyActorBase::GetItemOwner() const
{
	if (!DataSource.IsValid())
	{
		return nullptr;
	}

	return DataSource->GetItemOwner();
}

Faerie::ItemData::FProxyChangeEvent::RegistrationType& AFaerieProxyActorBase::GetOnProxyChangeEvent()
{
	if (!DataSource.IsValid())
	{
		checkNoEntry();
		static Faerie::ItemData::FProxyChangeEvent Blank;
		return Blank;
	}

	return DataSource.GetOnProxyChangeEvent();
}

void AFaerieProxyActorBase::SetSourceProxy(const FFaerieItemProxy& Source)
{
	if (Source != DataSource)
	{
		DataSource = Source;
		RegenerateDataDisplay();
	}
}
