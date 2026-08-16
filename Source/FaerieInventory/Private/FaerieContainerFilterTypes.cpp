// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieContainerFilterTypes.h"
#include "FaerieContainerIterator.h"

namespace Faerie::Container
{
	bool FCallbackFilter::Exec(const FMassEntityManager* EntityManager, const TValid<const FFaerieItemProxy&> Proxy) const
	{
		return Callback.Execute(EntityManager, Proxy);
	}
}
