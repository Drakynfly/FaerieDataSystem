// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "FaerieItemStackView.h"
#include "UObject/Interface.h"
#include "UObject/WeakInterfacePtr.h"

#include "FaerieItemProxy.generated.h"

class IFaerieItemOwnerInterface;
class UFaerieItem;

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
 * IFaerieItemDataProxy or its struct form FFaerieItemProxy and most anything can call that function.
 */
class FAERIEITEMDATA_API IFaerieItemDataProxy
{
	GENERATED_BODY()

public:
	// Get the Item Definition Object that this proxy represents.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual const UFaerieItem* GetItemObject() const PURE_VIRTUAL(IFaerieItemDataProxy::GetItemData, return nullptr; )

	// Get the number of copies this proxy may access.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual int32 GetCopies() const PURE_VIRTUAL(IFaerieItemDataProxy::GetCopies, return -1; )

	// Get the Object that owns the item this proxy represents.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetItemOwner() const PURE_VIRTUAL(IFaerieItemDataProxy::GetItemOwner, return nullptr; )

#if WITH_EDITOR
	// Stub for UFaerieItemAssetThumbnailRenderer to provide a thumbnail object for the editor.
	virtual class UThumbnailInfo* GetThumbnailInfo() const { return nullptr; }
#endif
};


// This struct contains a weak pointer to a proxy of a FaerieItem somewhere. This struct should never be
// serialized, and will not keep the proxy it points to alive.
// Access to the referenced item data is always const. Mutable access must be granted by the owner of the data.
USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/FaerieItemData.FaerieItemProxyUtils.ToWeakProxy"))
struct FAERIEITEMDATA_API FFaerieItemProxy
{
	GENERATED_BODY()

	FFaerieItemProxy() = default;

	FFaerieItemProxy(TYPE_OF_NULLPTR) {}

	FFaerieItemProxy(const IFaerieItemDataProxy* Interface)
	  : Proxy(Cast<UObject>(Interface)) {}

	template <Faerie::CItemDataProxy T>
	FFaerieItemProxy(const TObjectPtr<T> Interface)
	  : Proxy(Interface) {}

	FFaerieItemProxy(const TScriptInterface<IFaerieItemDataProxy>& Interface)
	  : Proxy(Interface.GetObject()) {}

private:
	UPROPERTY()
	TWeakObjectPtr<const UObject> Proxy;

public:
	bool IsValid() const;

	const UObject* GetObject() const
	{
		return Proxy.Get();
	}

	const UFaerieItem* GetItemObject() const;
	int32 GetCopies() const;
	TScriptInterface<IFaerieItemOwnerInterface> GetOwner() const;
	bool IsInstanceMutable() const;

	FORCEINLINE const IFaerieItemDataProxy* operator->() const { return Cast<IFaerieItemDataProxy>(Proxy.Get()); }

	explicit operator FFaerieItemStackView() const;

	friend bool operator==(const FFaerieItemProxy& Lhs, const FFaerieItemProxy& Rhs) { return Lhs.Proxy == Rhs.Proxy; }
	friend bool operator!=(const FFaerieItemProxy& Lhs, const FFaerieItemProxy& Rhs) { return !(Lhs == Rhs); }
};