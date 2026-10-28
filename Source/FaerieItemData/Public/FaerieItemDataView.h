// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataDefines.h"
#include "Mass/EntityHandle.h"

class UFaerieItem;

namespace Faerie::ItemData
{
	class IViewBase
	{
	public:
		virtual ~IViewBase() = default;

#if WITH_EDITOR
		// Get the item object from this view. Valid for editor access of an item.
		virtual const UFaerieItem* GetItemAsset() const = 0;
#endif

		// Get the item entity handle from this view. Valid for runtime access of an item.
		virtual FMassEntityHandle GetEntityHandle() const = 0;

		// Get the number of copies this view may access.
		virtual int32 GetCopies() const = 0;
	};
}

namespace Faerie::ItemData
{
	// Literal item data proxy interface for data on the stack
	struct FScopeProxy final : public IViewBase
	{
		FScopeProxy(TYPE_OF_NULLPTR)
		  : Copies(0) {}

		// Create a proxy that views a live item entity.
		FScopeProxy(const FMassEntityHandle ItemHandle, const int32 Copies)
		  : LiveItem(ItemHandle), Copies(Copies) {}

		// Create a proxy that views a default item asset.
		FScopeProxy(const TNotNull<const UFaerieItem*> ItemAsset, const int32 Copies)
		  : ItemAsset(ItemAsset), Copies(Copies) {}

	protected:
		// ReSharper disable CppOverrideWithDifferentVisibility
#if WITH_EDITOR
		UE_REWRITE virtual const UFaerieItem* GetItemAsset() const override { return ItemAsset; }
#endif
		UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return LiveItem; }
		UE_REWRITE virtual int32 GetCopies() const override { return Copies; }
		// ReSharper restore CppOverrideWithDifferentVisibility

	public:
		UE_REWRITE bool IsValid() const { return (ItemAsset || LiveItem.IsSet()) && IsValidStackAmount(Copies); }

		void SetCopies(const int32 InCopies) { Copies = InCopies; }

		TObjectPtr<const UFaerieItem> ItemAsset;
		FMassEntityHandle LiveItem;
		int32 Copies;
	};
}