// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"

namespace Faerie::Container
{
	// Run a callback on the iterator, allowing user code to run arbitrary selection logic.
	struct FAERIEINVENTORY_API FCallbackFilter
	{
		bool Exec(const FMassEntityManager* EntityManager, TValid<const FFaerieItemProxy&> Proxy) const;
		ItemData::FViewPredicate Callback;
	};
}
