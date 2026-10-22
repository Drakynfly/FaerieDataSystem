// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataDefines.h"
#include "FaerieItemInstance.h"

namespace Faerie::ItemData
{
	class IViewBase
	{
	public:
		virtual ~IViewBase() = default;

		// Get the item entity handle from this view.
		virtual FMassEntityHandle GetEntityHandle() const = 0;

		// Get the valid item instance or NullOpt from this view.
		virtual TOptional<FFaerieItemInstance> GetItemInstance() const = 0;

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

		FScopeProxy(const FFaerieItemInstance& Ref, const int32 Copies)
		  : Instance(Ref), Copies(Copies) {}

	protected:
		// ReSharper disable CppOverrideWithDifferentVisibility
		UE_REWRITE virtual FMassEntityHandle GetEntityHandle() const override { return Instance.GetMassEntityHandle(); }
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Instance; }
		UE_REWRITE virtual int32 GetCopies() const override { return Copies; }
		// ReSharper restore CppOverrideWithDifferentVisibility

	public:
		UE_REWRITE bool IsValid() const { return !Instance.IsEmpty() && IsValidStackAmount(Copies); }

		void SetCopies(const int32 InCopies) { Copies = InCopies; }

		FFaerieItemInstance Instance;
		int32 Copies;
	};
}