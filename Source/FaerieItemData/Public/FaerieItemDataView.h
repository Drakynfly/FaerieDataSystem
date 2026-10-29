// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Mass/EntityHandle.h"

class UFaerieItemAsset;

namespace Faerie::ItemData
{
	class IViewBase
	{
	public:
		virtual ~IViewBase() = default;

#if WITH_EDITOR
		// Get the item object from this view. Valid for editor access of an item.
		virtual const UFaerieItemAsset* GetItemAsset() const = 0;
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

#if WITH_EDITOR
		// Create a proxy that views a default item asset. Only allowed in the editor.
		FScopeProxy(const TNotNull<const UFaerieItemAsset*> ItemAsset, const int32 Copies)
		  : ItemAsset(ItemAsset), Copies(Copies) {}
#endif

	protected:
		// ReSharper disable CppOverrideWithDifferentVisibility
#if WITH_EDITOR
		UE_REWRITE virtual const UFaerieItemAsset* GetItemAsset() const override { return ItemAsset; }
#endif
		UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return LiveItem; }
		UE_REWRITE virtual int32 GetCopies() const override { return Copies; }
		// ReSharper restore CppOverrideWithDifferentVisibility

	public:
		FAERIEITEMDATA_API bool IsValid() const;

		void SetCopies(const int32 InCopies) { Copies = InCopies; }

#if WITH_EDITOR
		TObjectPtr<const UFaerieItemAsset> ItemAsset;
#endif
		FMassEntityHandle LiveItem;
		int32 Copies;
	};
}