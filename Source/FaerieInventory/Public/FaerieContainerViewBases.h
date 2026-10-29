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

	private:
#if WITH_EDITOR
		// As IFaerieContainerProxy is only implemented by runtime containers, it will never provide an item asset object.
		// ReSharper disable once CppOverrideWithDifferentVisibility
		virtual const UFaerieItemAsset* GetItemAsset() const override final { return nullptr; }
#endif
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
