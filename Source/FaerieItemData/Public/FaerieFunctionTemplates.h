// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "FaerieFunctionTemplates.generated.h"

struct FFaerieItemProxy;

/**
 * 
 */
UCLASS(Abstract, Const, Transient)
class FAERIEITEMDATA_API UFaerieFunctionTemplates : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFaerieProxyPredicate, const FFaerieItemProxy&, Proxy);
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieProxyComparator, const FFaerieItemProxy&, ProxyA, const FFaerieItemProxy&, ProxyB);

	static UFunction* GetFaerieProxyPredicateFunction();
	static UFunction* GetFaerieProxyComparatorFunction();
};