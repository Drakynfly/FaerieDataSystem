// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieFunctionTemplates.h"

#define FAERIE_GET_UFUNCTION(Function) UFaerieFunctionTemplates::StaticClass()->FindFunctionByName(UE_STRINGIZE(UE_JOIN(Function, __DelegateSignature)))

UFunction* UFaerieFunctionTemplates::GetFaerieItemPredicateFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieItemPredicate);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieStackPredicateFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieStackPredicate);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieSnapshotPredicateFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieSnapshotPredicate);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieItemComparatorFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieItemComparator);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieStackComparatorFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieStackComparator);
	return Function;
}

UFunction* UFaerieFunctionTemplates::GetFaerieSnapshotComparatorFunction()
{
	static UFunction* const Function = FAERIE_GET_UFUNCTION(FaerieSnapshotComparator);
	return Function;
}
