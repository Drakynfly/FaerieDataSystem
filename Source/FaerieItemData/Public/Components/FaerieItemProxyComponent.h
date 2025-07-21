// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemProxy.h"
#include "Components/ActorComponent.h"
#include "FaerieItemProxyComponent.generated.h"

class UFaerieItemProxyComponent;
using FItemSetEventNative = TMulticastDelegate<void(UFaerieItemProxyComponent*, FFaerieItemProxy)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemSetEvent, UFaerieItemProxyComponent*, Component, FFaerieItemProxy, Proxy);
using FItemClearEventNative = TMulticastDelegate<void(UFaerieItemProxyComponent*)>;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemClearEvent, UFaerieItemProxyComponent*, Component);


/**
 * A simple actor component that holds an item proxy struct, and offers events for the implementing actor to use.
 */
UCLASS(ClassGroup = ("Faerie"), meta = (BlueprintSpawnableComponent))
class FAERIEITEMDATA_API UFaerieItemProxyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFaerieItemProxyComponent();

	FItemSetEventNative::RegistrationType& GetOnItemSet() { return OnItemSetNative; }
	FItemClearEventNative::RegistrationType& GetOnItemClear() { return OnItemClearNative; }
	FFaerieItemProxy GetItemProxy() const { return ItemProxy; }

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemProxyComponent")
	void SetItemProxy(FFaerieItemProxy Proxy);

	UFUNCTION(BlueprintCallable, Category = "Faerie|ItemProxyComponent")
	void ClearItemProxy();

protected:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FItemSetEvent OnItemSet;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FItemClearEvent OnItemClear;

	UPROPERTY(BlueprintReadOnly, Category = "ItemProxy")
	FFaerieItemProxy ItemProxy;

private:
	FItemSetEventNative OnItemSetNative;
	FItemClearEventNative OnItemClearNative;
};