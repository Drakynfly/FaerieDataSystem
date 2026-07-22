// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemInstance.h"
#include "GameplayTagContainer.h"

#include "UObject/Interface.h"

#include "FaerieItemProxy.generated.h"

class IFaerieItemOwnerInterface;

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
{
	GENERATED_BODY()

public:
	// Get the valid item instance or NullOpt from this proxy.
	virtual TOptional<FFaerieItemInstance> GetItemInstance() const = 0;

	// Get the number of copies this proxy may access.
	virtual int32 GetCopies() const = 0;

	// Get the Proxy Interface Object that points to the item this proxy represents.
	virtual IFaerieItemOwnerInterface* GetItemOwner() const = 0;

	// Get the multicast delegate for listening to changes to the item data of this proxy.
	virtual Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() = 0;

#if WITH_EDITOR
	// Stub for UFaerieItemAssetThumbnailRenderer to provide a thumbnail object for the editor.
	virtual class UThumbnailInfo* GetThumbnailInfo() const { return nullptr; }
#endif
};

// This struct contains a pointer to a proxy of a FaerieItem somewhere. This struct should never be
// serialized.
// Access to the referenced item data is always const. Mutable access must be granted by the owner of the data.
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/FaerieItemData.FaerieItemProxyUtils.ToWeakProxy"))
struct FAERIEITEMDATA_API FFaerieItemProxy
{
	GENERATED_BODY()

	FFaerieItemProxy() = default;

	FFaerieItemProxy(const FFaerieItemProxy& Other)
	  : Proxy(Other.Proxy) {}

	explicit FFaerieItemProxy(const TScriptInterface<const IFaerieItemDataProxy>& Interface)
	  : Proxy(Interface.GetObject()) {}

private:
	// Storing the Proxy object as a ScriptInterface requires storing 2 pointers, but allows direct access to the interface
	// while also keeping a strong pointer to the UObject.
	UPROPERTY()
	TScriptInterface<const IFaerieItemDataProxy> Proxy;

public:
	bool IsValid() const;

	// Get the UObject that implements the IFaerieItemDataProxy interface.
	UE_REWRITE const UObject* GetProxyObject() const { return Proxy.GetObject(); }

	template <typename T>
	UE_REWRITE const T* GetTypedProxyObject() const { return Cast<T>(Proxy.GetObject()); }

	// Get the Interface pointer.
	UE_REWRITE const IFaerieItemDataProxy* GetInterface() const { return Proxy.GetInterface(); }

	const IFaerieItemDataProxy* operator->() const;

	[[nodiscard]] UE_REWRITE bool UEOpEquals(const FFaerieItemProxy& Other) const
	{
		return Proxy == Other.Proxy;
	}

	friend [[nodiscard]] UE_REWRITE uint32 GetTypeHash(const FFaerieItemProxy& Proxy)
	{
		return GetTypeHash(Proxy.Proxy);
	}

	// Utility to access the change event, which requires non-const access to the delegate, and proxies are always
	// treated as const, so a quick little ugly workaround is needed.
	Faerie::ItemData::FProxyChangeEvent::RegistrationType& GetOnProxyChangeEvent() const;
};