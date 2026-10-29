// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataView.h"
#include "FaerieItemAsset.h"

namespace Faerie::ItemData
{
	bool FScopeProxy::IsValid() const
	{
		if (!IsValidStackAmount(Copies))
		{
			return false;
		}

#if WITH_EDITOR
		if (::IsValid(ItemAsset))
		{
			return true;
		}
#endif
		return LiveItem.IsSet();
	}
}
