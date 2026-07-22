// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "FaerieFunctionTemplates.generated.h"

struct FFaerieItemDataView;

/**
 * 
 */
UCLASS(Abstract, Const, Transient)
class FAERIEITEMDATA_API UFaerieFunctionTemplates : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieViewPredicate, const UObject*, WorldContextObj, const FFaerieItemDataView&, View);
	DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FFaerieViewComparator, const UObject*, WorldContextObj, const FFaerieItemDataView&, ViewA, const FFaerieItemDataView&, ViewB);

	static UFunction* GetFaerieViewPredicateFunction();
	static UFunction* GetFaerieViewComparatorFunction();
};