// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerIterator.h"
#include "FaerieItemContainerBase.h"

namespace Faerie::Container
{
	FVirtualKeyIterator KeyRange(const UFaerieItemContainerBase* Container)
	{
		return FVirtualKeyIterator(Container->CreateIterator(false));
	}

	FVirtualAddressIterator AddressRange(const UFaerieItemContainerBase* Container)
	{
		return FVirtualAddressIterator(Container->CreateIterator(true));
	}

	FVirtualConstItemIterator ItemRange(const UFaerieItemContainerBase* Container)
	{
		return FVirtualConstItemIterator(Container->CreateIterator(false));
	}
}
