// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Actors/FaerieProxyActorBase.h"
#include "FaerieMassFragmentAccessors.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieProxyActorBase)

AFaerieProxyActorBase::AFaerieProxyActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

#if WITH_EDITOR
const UFaerieItemAsset* AFaerieProxyActorBase::GetItemAsset() const
{
	return DataSource.GetItemPtr();
}
#endif

FMassEntityHandle AFaerieProxyActorBase::GetEntityHandle() const
{
	return DataSource.GetEntityHandle();
}

int32 AFaerieProxyActorBase::GetCopies() const
{
	return DataSource.GetCopies();
}

void AFaerieProxyActorBase::SetSourceProxy(const FFaerieItemProxy& Source)
{
	if (Source != DataSource)
	{
		DataSource = Source;
		RegenerateDataDisplay();
	}
}
