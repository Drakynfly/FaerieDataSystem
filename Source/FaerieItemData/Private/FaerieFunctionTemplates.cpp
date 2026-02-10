// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieFunctionTemplates.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieFunctionTemplates)

#define FAERIE_GET_UFUNCTION(Function) UFaerieFunctionTemplates::StaticClass()->FindFunctionByName(UE_STRINGIZE(UE_JOIN(Function, __DelegateSignature)))

UFunction* UFaerieFunctionTemplates::GetFaerieViewPredicateFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieViewPredicate);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieViewComparatorFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieViewComparator);
	return Function;
}
