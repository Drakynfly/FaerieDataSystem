// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "FaerieItemInstance.h"
#include "GameplayTagContainer.h"
#include "ValidParameter.h"

#include "UObject/Interface.h"

#include "FaerieItemProxy.generated.h"

namespace Faerie::ItemData
{
	using FProxyChangeEvent = TMulticastDelegate<void(const FFaerieItemProxy&, FGameplayTag)>;
}

// @todo Eventually this should not be BlueprintType, once all APIs use FFaerieItemProxy
UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class FAERIEITEMDATA_API UFaerieItemDataProxy : public UInterface
{
	GENERATED_BODY()
};

/**
 * Item Data Proxies are objects to pass around item data, without breaking ownership.
 * There are multiple implementations for various purposes, but their primary point is to allow API's to be created
 * without having to worry about the various forms items can come in. Just declare a function that takes an
 * IFaerieItemDataProxy in its struct form FFaerieItemProxy and most anything can call that function.
 */
class FAERIEITEMDATA_API IFaerieItemDataProxy
#if CPP
	: public Faerie::ItemData::IViewBase
#endif
{
	GENERATED_BODY()

public:
	// Get the multicast delegate for listening to changes to the item data of this proxy.
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() = 0;
};

// This struct contains a pointer to a proxy of a FaerieItem somewhere. This struct should never be
// serialized.
// Access to the referenced item data is always const. Mutable access must be granted by the owner of the data.
USTRUCT(BlueprintType)
struct FAERIEITEMDATA_API FFaerieItemProxy
{
	GENERATED_BODY()

	FFaerieItemProxy() = default;

	FFaerieItemProxy(const FFaerieItemProxy& Other)
	  : ProxyObject(Other.ProxyObject), InterfacePtr(Other.InterfacePtr) {}

	// Construct from a persistable pointer.
	explicit FFaerieItemProxy(const TScriptInterface<const IFaerieItemDataProxy>& Interface)
	  : ProxyObject(Interface.GetObject()), InterfacePtr(Interface.GetInterface()) {}

	// Constructor for single frame 'view' proxies that are cheaper than persistent proxies since they don't need a
	// UObject backing them.
	enum ECtorOverride { ESingleFrame };
	explicit FFaerieItemProxy(ECtorOverride, const TNotNull<const Faerie::ItemData::IViewBase*> View UE_LIFETIMEBOUND)
	  : ProxyObject(nullptr), InterfacePtr(View) {}

private:
	// UObject to keep alive *if* this proxy refers to one.
	UPROPERTY()
	TObjectPtr<const UObject> ProxyObject;

	// Pointer to the interface. If ProxyObject is valid, this also will be, but not always the other way around.
	// For single frame proxies, only this may be set.
	const Faerie::ItemData::IViewBase* InterfacePtr = nullptr;

public:
	// Only tests if this proxy has a valid interface pointer. It does not check if the data it points to is valid.
	bool IsValid() const;

	// Test if this proxy is valid and points to a valid item stack.
	bool HasValidInstance() const;

	// Will this proxy safely refer to the same data if kept alive across multiple frame. If this returns false, do not
	// store this proxy as a member.
	UE_REWRITE bool IsSafeToPersist() const { return !!ProxyObject; }

	// Get the UObject that implements the IFaerieItemDataProxy interface.
	// WARNING: May not be valid, even if IsValid returns true.
	UE_REWRITE const UObject* GetProxyObject() const { return ProxyObject; }

	template <typename T>
	UE_REWRITE const T* GetTypedProxyObject() const { return Cast<T>(ProxyObject); }

	// Get the Interface pointer.
	UE_REWRITE const Faerie::ItemData::IViewBase* GetInterface() const { return InterfacePtr; }

	// Get the valid item instance or NullOpt from this proxy.
	TOptional<FFaerieItemInstance> GetItemInstance() const;

	// Get the valid item instance or a blank instance.
	FFaerieItemInstance GetItemInstanceOrInvalid() const;

	// Get the number of copies this proxy may access.
	int32 GetCopies() const;

	// Get the owning object for the item this proxy represents.
	UObject* GetItemOwner() const;

	// Utility to access the change event, which requires non-const access to the delegate, and proxies are always
	// treated as const, so a quick little ugly workaround is needed.
	Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() const;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemProxy& Other) const
	{
		return InterfacePtr == Other.InterfacePtr;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemProxy& Proxy)
	{
		return GetTypeHash(Proxy.InterfacePtr);
	}
};

namespace Faerie::ItemData
{
	// Typedef for delegates that consume predicate functions.
	using FViewPredicate = TDelegate<bool(const FMassEntityManager* EntityManager, TValid<const FFaerieItemProxy&>)>;

	// Typedef for delegate that consume comparison functions.
	using FViewComparator = TDelegate<bool(const FMassEntityManager* EntityManager, TValid<const FFaerieItemProxy&>, TValid<const FFaerieItemProxy&>)>;
}