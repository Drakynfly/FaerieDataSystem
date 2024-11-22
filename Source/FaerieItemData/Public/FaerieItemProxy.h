﻿// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

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
	virtual const UFaerieItem* GetItemObject() const PURE_VIRTUAL(UFaerieItemDataProxy::GetItemData, return nullptr; )

	// Get the number of copies this proxy may access.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual int32 GetCopies() const PURE_VIRTUAL(UFaerieItemDataProxy::GetCopies, return -1; )

	// KINDA TEMP. Can the Item be modified through this proxy. NO NON-CONST ACCESSOR CURRENTLY EXISTS.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual bool CanMutate() const PURE_VIRTUAL(UFaerieItemDataProxy::CanMutate, return false; )

	// Get the Object that owns the item this proxy represents.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemDataProxy")
	virtual TScriptInterface<IFaerieItemOwnerInterface> GetOwner() const PURE_VIRTUAL(UFaerieItemDataProxy::GetOwner, return nullptr; )
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
	  : Proxy(Interface->_getUObject()) {}

	template<typename T>
	FFaerieItemProxy(const TObjectPtr<T> DerivedProxy, 
					 typename std::enable_if<std::is_base_of<IFaerieItemDataProxy, T>::value>::type* = nullptr)
		: Proxy(DerivedProxy) {}

	FFaerieItemProxy(const TScriptInterface<IFaerieItemDataProxy>& Interface)
	  : Proxy(Interface.GetObject()) {}

private:
	UPROPERTY()
	TWeakObjectPtr<const UObject> Proxy;

public:
	bool IsValid() const
	{
		return Proxy.IsValid();
	}

	const UObject* GetObject() const
	{
		return Proxy.Get();
	}

	const UFaerieItem* GetItemObject() const;
	int32 GetCopies() const;
	TScriptInterface<IFaerieItemOwnerInterface> GetOwner() const;

	const IFaerieItemDataProxy* operator->() const { return Cast<IFaerieItemDataProxy>(Proxy.Get()); }

	friend bool operator==(const FFaerieItemProxy& Lhs, const FFaerieItemProxy& Rhs) { return Lhs.Proxy == Rhs.Proxy; }
	friend bool operator!=(const FFaerieItemProxy& Lhs, const FFaerieItemProxy& Rhs) { return !(Lhs == Rhs); }
};