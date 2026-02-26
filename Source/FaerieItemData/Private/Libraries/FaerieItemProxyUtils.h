// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "FaerieItemStackView.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FaerieItemProxyUtils.generated.h"

class IFaerieItemOwnerInterface;
class UFaerieItem;

/**
 *
 */
UCLASS()
class UFaerieItemProxyUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Attempt to cast a weak proxy struct (interface pointer) into a typed proxy object pointer.
	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemProxyUtils", meta = (DeterminesOutputType = "Class", DynamicOutputParam = "ProxyObject", ExpandBoolAsExecs = "ReturnValue"))
	static bool CastProxy(const FFaerieItemProxy& Proxy, UPARAM(meta = (MustImplement = "/Script/FaerieItemData.FaerieItemDataProxy")) UClass* Class, UObject*& ProxyObject);

	// Make a weak proxy struct from a typed proxy object.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static FFaerieItemProxy ToWeakProxy(const TScriptInterface<IFaerieItemDataProxy>& ScriptProxy);

	// Get the Object implementing the IFaerieItemDataProxy interface.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static const UObject* GetProxyObject(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static bool IsValid(const FFaerieItemProxy& Proxy);

	// Get the Item Definition Object that this proxy represents.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static const UFaerieItem* GetItemObject(const FFaerieItemProxy& Proxy);

	// Get the number of copies this proxy may access.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static int32 GetCopies(const FFaerieItemProxy& Proxy);

	// Get the Object that owns the item this proxy represents.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static TScriptInterface<IFaerieItemOwnerInterface> GetOwner(const FFaerieItemProxy& Proxy);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemProxyUtils")
	static void BindToItemDataChanged(const FFaerieItemProxy& Proxy, const FFaerieItemProxyChangedEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemProxyUtils")
	static void UnbindAllFromItemDataChanged(const FFaerieItemProxy& Proxy, const UObject* Object);

	// Convert an Item Proxy into a Stack View.
	UFUNCTION(BlueprintPure, Category = "Faerie|ItemProxyUtils")
	static FFaerieItemStackView ProxyToView(const FFaerieItemProxy& Proxy);
};