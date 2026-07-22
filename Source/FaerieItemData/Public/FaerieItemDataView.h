// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataDefines.h"
#include "FaerieItemInstance.h"
#include "FaerieItemProxy.h"

#include "FaerieItemDataView.generated.h"

namespace Faerie::ItemData
{
	static bool IsValidStackAmount(const int32 Value)
	{
		return Value > 0 || Value == UnlimitedStack;
	}

	class IViewBase
	{
	public:
		virtual ~IViewBase() = default;

		virtual FReference ResolveItem() const = 0;
		virtual int32 ResolveCopies() const = 0;
		virtual const IFaerieItemOwnerInterface* ResolveOwner() const = 0;
	};
}

namespace Faerie::ItemData
{
	struct [[nodiscard]] FNonNullViewPtr
	{
		FNonNullViewPtr(const IViewBase* Ptr)
		  : ImplPtr(Ptr) {}

		FNonNullViewPtr(const IViewBase& Ref)
		  : ImplPtr(&Ref) {}

		TNotNull<const IViewBase*> ImplPtr;

		UE_REWRITE const IViewBase* operator->() const { return ImplPtr; }

		[[nodiscard]] UE_REWRITE bool UEOpEquals(const FNonNullViewPtr& Other) const
		{
			return ImplPtr == Other.ImplPtr;
		}
	};

	// @todo constrain T to impl of IViewBase
	template <typename T>
	struct [[nodiscard]] TNonNullViewPtr
	{
		TNonNullViewPtr() = default;
		TNonNullViewPtr(const TNotNull<const T*> Pointer)
		  : InnerPtr(Pointer) {}
		TNonNullViewPtr(const T& Ref UE_LIFETIMEBOUND)
		  : InnerPtr(&Ref) {}

		UE_REWRITE TNotNull<const T*> operator->() const { return (T*)NotNullGet(InnerPtr.ImplPtr); }
		UE_REWRITE const FNonNullViewPtr& operator*() const { return InnerPtr; }

	private:
		FNonNullViewPtr InnerPtr;
	};

	// A reference to a faerie item that is validated for the current scope.
	struct [[nodiscard]] FAERIEITEMDATA_API FReference
	{
		template <
			typename T
			UE_REQUIRES(std::is_convertible_v<T, FFaerieItemInstance>)
		>
		FReference(T Instance)
		  : Instance(Instance)
		{
			check(this->Instance.IsValid())
		}

		UE_REWRITE const FFaerieItemInstance* operator->() const { return &Instance; }

		[[nodiscard]] UE_REWRITE const FFaerieItemInstance& GetInstance() const { return Instance; }

		[[nodiscard]] UE_REWRITE operator const FFaerieItemInstance&() const { return Instance; }

		[[nodiscard]] UE_REWRITE bool UEOpEquals(const FReference& Other) const
		{
			return Instance == Other.Instance;
		}

	protected:
		FFaerieItemInstance Instance;
	};

	// A reference to a mutable faerie item that is validated for the current scope.
	struct [[nodiscard]] FAERIEITEMDATA_API FMutableReference : FReference
	{
		FMutableReference(const FReference& Other)
		  : FReference(Other)
		{
			if (!Instance.IsMutable())
			{
				UE_LOG(LogCore, Fatal, TEXT("Immutable instance assigned to Faerie::ItemData::FMutableReference"));
			}
		}

		template <
			typename T
			UE_REQUIRES(std::is_convertible_v<T, FFaerieItemInstance>)
		>
		FMutableReference(T Instance)
		  : FReference(Instance)
		{
			if (!Instance.IsMutable())
			{
				UE_LOG(LogCore, Fatal, TEXT("Immutable instance assigned to Faerie::ItemData::FMutableReference"));
			}
		}

		// Manual ctor for client-side purposes. Temporary.
		enum ECtor { BypassMutateCast };
		UE_REWRITE FMutableReference(ECtor, const FFaerieItemInstance& Item) : FReference(Item) {}

		UE_REWRITE FFaerieItemInstance* operator->() const { return &const_cast<FFaerieItemInstance&>(Instance); }
		[[nodiscard]] UE_REWRITE FFaerieItemInstance& GetInstance() const { return const_cast<FFaerieItemInstance&>(Instance); }

#if WITH_EDITOR
		// The Editor can use a MutableReference to directly read the instance's item asset.
		UE_REWRITE TNotNull<UFaerieItem*> GetMutableItemPtr() const { return const_cast<UFaerieItem*>(Instance.GetItemPtr()); }
#endif
	};
}

/**
 * A view of item data, comprised of an instance, number of copies, and an owner for them if relevant.
 * Data is cached once fetched, and not fetched until needed.
 */
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemDataView
{
	GENERATED_BODY()

	using FValidatedProxy = TNotNull<const IFaerieItemDataProxy*>;
	using FExternalResolver = Faerie::ItemData::FNonNullViewPtr;

	FFaerieItemDataView() = default;

	// Make a data view using a ItemData::ViewBase to resolve data from.
	UE_REWRITE FFaerieItemDataView(const Faerie::ItemData::FNonNullViewPtr& Proxy)
	  : Resolver(TInPlaceType<FExternalResolver>(), Proxy) {}

	// Make a data view from a proxy struct that we can store.
	UE_REWRITE FFaerieItemDataView(TNotNull<const IFaerieItemDataProxy*> Proxy)
	  : Resolver(TInPlaceType<FValidatedProxy>(), MoveTemp(Proxy)) {}

	// Make a data view from a proxy struct that we can store.
	UE_REWRITE FFaerieItemDataView(const FFaerieItemProxy& Proxy)
	  : Resolver(TInPlaceType<FValidatedProxy>(), Proxy.GetInterface()) {}

	// Make a data view from existing fetched data.
	UE_REWRITE FFaerieItemDataView(const Faerie::ItemData::FReference& Item, const int32 Copies, const IFaerieItemOwnerInterface* Owner)
		: Resolver(TInPlaceType<FEmptyVariantState>())
	{
		CachedInstance = Item;
		CachedCopies = Copies;
		CachedOwner = Owner;
	}

private:
	TVariant<FEmptyVariantState, FValidatedProxy, FExternalResolver> Resolver;

	mutable TOptional<Faerie::ItemData::FReference> CachedInstance;
	mutable TOptional<int32> CachedCopies;
	mutable TOptional<const IFaerieItemOwnerInterface*> CachedOwner;

public:
	// Validator. This will cache the Instance and Copies if not already.
	UE_REWRITE bool IsValid() const { return GetInstance().IsValid() && Faerie::ItemData::IsValidStackAmount(GetCopies()); }
	UE_REWRITE operator bool() const { return IsValid(); }

	void SetItemObject(const Faerie::ItemData::FReference& Item);
	void SetCopies(int32 Copies);
	void SetOwner(TNotNull<const IFaerieItemOwnerInterface*> Owner);

	FFaerieItemInstance GetInstance() const;
	int32 GetCopies() const;
	const IFaerieItemOwnerInterface* GetOwner() const;
};

template<> struct TStructOpsTypeTraits<FFaerieItemDataView> : TStructOpsTypeTraitsBase2<FFaerieItemDataView>
{
	enum { WithCopy = false };
};

namespace Faerie::ItemData
{
	struct [[nodiscard]] FValidatedDataView
	{
		FValidatedDataView(const FFaerieItemDataView* ViewPtr)
		  : DataView(*ViewPtr)
		{
			check(DataView.IsValid());
		}

		FValidatedDataView(const FFaerieItemDataView& ViewRef)
		  : DataView(ViewRef)
		{
			check(DataView.IsValid());
		}

		const FFaerieItemDataView* operator->() const { return &DataView; }
		operator const FFaerieItemDataView&() const { return DataView; }

		const FFaerieItemDataView& DataView;
	};

	// Typedef for delegates that consume predicate functions.
	using FViewPredicate = TDelegate<bool(TNotNull<const UObject*> WorldContextObj, const FValidatedDataView&)>;

	// Typedef for delegate that consume comparison functions.
	using FViewComparator = TDelegate<bool(TNotNull<const UObject*> WorldContextObj, const FValidatedDataView&, const FValidatedDataView&)>;
}