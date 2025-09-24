// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemContainerStructs.h"
#include "FaerieItemContainerBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemContainerStructs)

bool FFaerieAddressableHandle::IsValid() const
{
	return Container.IsValid() && Container->Contains(Address);
}

FFaerieItemProxy FFaerieAddressableHandle::ToProxy() const
{
	return FFaerieItemProxy(Container->Proxy(Address));
}
