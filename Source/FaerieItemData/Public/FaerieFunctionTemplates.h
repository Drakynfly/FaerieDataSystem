// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "FaerieFunctionTemplates.generated.h"

struct FFaerieItemStackView;
struct FFaerieItemSnapshot;

/**
 * 
 */
UCLASS(Abstract, Const, Transient)
class FAERIEITEMDATA_API UFaerieFunctionTemplates : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFaerieItemPredicate, const UFaerieItem*, Item);
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFaerieStackPredicate, const FFaerieItemStackView&, Stack);
	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FFaerieSnapshotPredicate, const FFaerieItemSnapshot&, Snapshot);

	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieItemComparator, const UFaerieItem*, ItemA, const UFaerieItem*, ItemB);
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieStackComparator, const FFaerieItemStackView&, StackA, const FFaerieItemStackView&, StackB);
	DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FFaerieSnapshotComparator, const FFaerieItemSnapshot&, SnapshotA, const FFaerieItemSnapshot&, SnapshotB);

	static UFunction* GetFaerieItemPredicateFunction();
	static UFunction* GetFaerieStackPredicateFunction();
	static UFunction* GetFaerieSnapshotPredicateFunction();
	static UFunction* GetFaerieItemComparatorFunction();
	static UFunction* GetFaerieStackComparatorFunction();
	static UFunction* GetFaerieSnapshotComparatorFunction();
};