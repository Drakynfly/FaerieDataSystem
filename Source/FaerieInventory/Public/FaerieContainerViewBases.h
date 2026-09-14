// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemContainerStructs.h"
#include "FaerieItemDataView.h"

namespace Faerie::Container
{
	/*
	 * Extend ViewBase to provide access to an EntryKey
	 */
	class IEntryView : public ItemData::IViewBase
	{
	public:
		virtual FFaerieEntryKey ResolveKey() const = 0;
	};

	/*
	 * Extend EntryView to provide access to an Address
	 */
	class IAddressView : public IEntryView
	{
	public:
		virtual FFaerieAddress ResolveAddress() const = 0;
	};
}
