// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataView.h"
#include "GameplayTagContainer.h"
#include "ValidParameter.h"

#include "UObject/Interface.h"

#include "FaerieItemProxy.generated.h"

namespace Faerie::ItemData
{
	// #todo remove or move this type
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
	  : ProxyObject(Other.ProxyObject), EntityManager(Other.EntityManager), InterfacePtr(Other.InterfacePtr), ValidWithoutProxyObject(Other.ValidWithoutProxyObject) {}

	// Construct from a persistable pointer.
	explicit FFaerieItemProxy(const TSharedPtr<FMassEntityManager>& EntityManager, const TScriptInterface<const IFaerieItemDataProxy>& Interface)
	  : ProxyObject(Interface.GetObject()), EntityManager(EntityManager), InterfacePtr(Interface.GetInterface()), ValidWithoutProxyObject(false) {}

	// Constructor for single frame 'view' proxies that are cheaper than persistent proxies since they don't need a
	// UObject backing them.
	enum ECtorOverride { ESingleFrame };
	explicit FFaerieItemProxy(ECtorOverride, const TSharedPtr<FMassEntityManager>& EntityManager, const TNotNull<const Faerie::ItemData::IViewBase*> View UE_LIFETIMEBOUND)
	  : ProxyObject(nullptr), EntityManager(EntityManager), InterfacePtr(View), ValidWithoutProxyObject(true) {}

private:
	// UObject to keep alive *if* this proxy refers to one.
	UPROPERTY()
	TObjectPtr<const UObject> ProxyObject;

	// Entity manager to access item data.
	TSharedPtr<FMassEntityManager> EntityManager;

	// Pointer to the interface. If ProxyObject is valid, this also will be, but not always the other way around.
	// For single frame proxies, only this may be set.
	const Faerie::ItemData::IViewBase* InterfacePtr = nullptr;

	// @todo hack until better solution for detecting invalid proxies
	bool ValidWithoutProxyObject = false;

public:
	// Only tests if this proxy has a valid interface pointer. It does not check if the data it points to is valid.
	// Used by Faerie::TValid
	bool IsValid() const;

	// Test if this proxy is valid and points to a valid item stack.
	bool HasValidInstance() const;

	bool HasEntityManager() const { return EntityManager.IsValid(); }

	// Will this proxy safely refer to the same data if kept alive across multiple frame. If this returns false, do not
	// store this proxy as a member.
	UE_REWRITE bool IsSafeToPersist() const { return !!ProxyObject; }

	// Get the UObject that implements the IFaerieItemDataProxy interface.
	// WARNING: May not be valid, even if IsValid returns true.
	UE_REWRITE const UObject* GetProxyObject() const { return ProxyObject; }

	template <typename T>
	UE_REWRITE const T* GetTypedProxyObject() const { return Cast<T>(ProxyObject); }

	const TSharedPtr<FMassEntityManager>& GetEntityManager() const { return EntityManager; }

	// Get the MassEntityHandle for this proxy. Can return an invalid handle if we proxy an editor/unregistered instance.
	FMassEntityHandle GetEntityHandle() const;

	// Get the valid item instance or NullOpt from this proxy.
	TOptional<FFaerieItemInstance> GetItemInstance() const;

	// Get the valid item instance or a blank instance.
	FFaerieItemInstance GetItemInstanceOrInvalid() const;

	// Get the number of copies this proxy may access.
	int32 GetCopies() const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bSuccess);

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemProxy& Other) const
	{
		return InterfacePtr == Other.InterfacePtr;
	}

	[[nodiscard]] UE_REWRITE friend uint32 GetTypeHash(const FFaerieItemProxy& Proxy)
	{
		return GetTypeHash(Proxy.InterfacePtr);
	}
};

template <>
struct TStructOpsTypeTraits<FFaerieItemProxy> : public TStructOpsTypeTraitsBase2<FFaerieItemProxy>
{
	enum
	{
		WithNetSerializer = true,
	};
};

namespace Faerie::ItemData
{
	// Typedef for delegates that consume predicate functions.
	using FViewPredicate = TDelegate<bool(TValid<const FFaerieItemProxy&>)>;

	// Typedef for delegate that consume comparison functions.
	using FViewComparator = TDelegate<bool(TValid<const FFaerieItemProxy&>, TValid<const FFaerieItemProxy&>)>;
}