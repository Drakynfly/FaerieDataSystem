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

		// Get the valid item instance or NullOpt from this proxy.
		virtual TOptional<FFaerieItemInstance> GetItemInstance() const = 0;

		// Get the number of copies this proxy may access.
		virtual int32 GetCopies() const = 0;

		// Get the Proxy Interface Object that points to the item this proxy represents.
		virtual const IFaerieItemOwnerInterface* GetItemOwner() const = 0;
	};
}

namespace Faerie::ItemData
{
	// Literal item data proxy interface for data on the stack
	struct FScopeProxy final : public IViewBase
	{
		FScopeProxy(TYPE_OF_NULLPTR)
		  : Copies(0), ItemOwner(nullptr) {}

		FScopeProxy(const FFaerieItemInstance& Ref, const int32 Copies, const IFaerieItemOwnerInterface* ItemOwner)
		  : Instance(Ref), Copies(Copies), ItemOwner(ItemOwner) {}

	protected:
		// ReSharper disable CppOverrideWithDifferentVisibility
		UE_REWRITE virtual TOptional<FFaerieItemInstance> GetItemInstance() const override { return Instance; }
		UE_REWRITE virtual int32 GetCopies() const override { return Copies; }
		UE_REWRITE virtual const IFaerieItemOwnerInterface* GetItemOwner() const override { return ItemOwner; }
		// ReSharper restore CppOverrideWithDifferentVisibility

	public:
		UE_REWRITE bool IsValid() const { return !Instance.IsEmpty() && IsValidStackAmount(Copies); }

		void SetCopies(const int32 InCopies) { Copies = InCopies; }

		FFaerieItemInstance Instance;
		int32 Copies;
		const IFaerieItemOwnerInterface* ItemOwner;
	};
}