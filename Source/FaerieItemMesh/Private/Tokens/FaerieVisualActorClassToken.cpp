// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Tokens/FaerieVisualActorClassToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieVisualActorClassToken)

TSubclassOf<AFaerieItemOwningActorBase> UFaerieVisualActorClassToken::LoadOwningActorClassSynchronous() const
{
	return OwningActorClass.LoadSynchronous();
}

TSubclassOf<AFaerieProxyActorBase> UFaerieVisualActorClassToken::LoadProxyActorClassSynchronous() const
{
	return ProxyActorClass.LoadSynchronous();
}
