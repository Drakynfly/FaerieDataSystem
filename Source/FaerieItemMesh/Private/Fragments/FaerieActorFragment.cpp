// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Fragments/FaerieActorFragment.h"

#include "Actors/FaerieItemOwningActorBase.h"
#include "Actors/FaerieProxyActorBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieActorFragment)

TSubclassOf<AFaerieItemOwningActorBase> FFaerieActorFragment::LoadOwningActorClassSynchronous() const
{
	return OwningActorClass.LoadSynchronous();
}

TSubclassOf<AFaerieProxyActorBase> FFaerieProxyActorFragment::LoadProxyActorClassSynchronous() const
{
	return ProxyActorClass.LoadSynchronous();
}