// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieFunctionTemplates.h"
#include "UObject/Class.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieFunctionTemplates)

#define FAERIE_GET_UFUNCTION(Function) UFaerieFunctionTemplates::StaticClass()->FindFunctionByName(UE_STRINGIZE(UE_JOIN(Function, __DelegateSignature)))

UFunction* UFaerieFunctionTemplates::GetFaerieProxyPredicateFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieProxyPredicate);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieProxyComparatorFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieProxyComparator);
	return Function;
}
