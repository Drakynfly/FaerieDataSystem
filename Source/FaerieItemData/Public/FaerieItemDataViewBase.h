// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemStackView.h"

class IFaerieItemOwnerInterface;
class UFaerieItem;

namespace Faerie::ItemData
{
	class IViewBase
	{
	public:
		virtual ~IViewBase() = default;

		virtual bool IsValid() const = 0;

		virtual const UFaerieItem* ResolveItem() const = 0;
		virtual FFaerieItemStackView ResolveView() const = 0;
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const = 0;
	};

	// Typedef for the rather ungainly parameter for filter predicates.
	using FViewPtr = const TNotNull<const IViewBase*>;

	// Typedef for delegates that consume predicate functions.
	using FViewPredicate = TDelegate<bool(FViewPtr)>;

	// Typedef for delegate that consume comparison functions.
	using FViewComparator = TDelegate<bool(FViewPtr, const FViewPtr)>;
}
