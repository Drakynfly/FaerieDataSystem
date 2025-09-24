// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerIterator.h"
#include "FaerieItemContainerBase.h"

namespace Faerie
{
	FDefaultKeyIterator KeyRange(const UFaerieItemContainerBase* Container)
	{
		return FDefaultKeyIterator(Container->CreateIterator());
	}

	FDefaultAddressIterator AddressRange(const UFaerieItemContainerBase* Container)
	{
		return FDefaultAddressIterator(Container->CreateIterator());
	}

	FDefaultConstItemIterator ItemRange(const UFaerieItemContainerBase* Container)
	{
		return FDefaultConstItemIterator(Container->CreateIterator());
	}
}
