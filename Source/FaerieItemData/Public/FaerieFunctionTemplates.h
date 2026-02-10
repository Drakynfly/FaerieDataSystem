// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "FaerieFunctionTemplates.generated.h"

struct FFaerieItemDataViewWrapper;

/**
 * 
 */
UCLASS(Abstract, Const, Transient)
class FAERIEITEMDATA_API UFaerieFunctionTemplates : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFaerieViewPredicate, const FFaerieItemDataViewWrapper&, View);
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieViewComparator, const FFaerieItemDataViewWrapper&, ViewA, const FFaerieItemDataViewWrapper&, ViewB);

	static UFunction* GetFaerieViewPredicateFunction();
	static UFunction* GetFaerieViewComparatorFunction();
};