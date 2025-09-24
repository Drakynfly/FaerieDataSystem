// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilter.h"
#include "FaerieItemContainerBase.h"

namespace Faerie
{
	FContainerKeyFilter KeyFilter(const UFaerieItemContainerBase* Container)
	{
		return FContainerKeyFilter(Container->CreateFilter(false));
	}

	FContainerAddressFilter AddressFilter(const UFaerieItemContainerBase* Container)
	{
		return FContainerAddressFilter(Container->CreateFilter(true));
	}
}
