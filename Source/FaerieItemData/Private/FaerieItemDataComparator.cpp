// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataComparator.h"
#include "FaerieItemDataView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataComparator)

bool UFaerieItemDataComparator::K2_Exec(UObject* WorldContextObj, const FFaerieItemDataView& A, const FFaerieItemDataView& B) const
{
	if (!IsValid(WorldContextObj))
	{
		FFrame::KismetExecutionMessage(TEXT("Invalid WorldContextObj passed to UFaerieItemDataComparator::K2_Exec"), ELogVerbosity::Error);
		return false;
	}

	if (A.IsValid() && B.IsValid())
	{
		return Exec(WorldContextObj, A, B);
	}
	return false;
}

bool UFaerieItemDataComparator_BlueprintBase::Exec(TNotNull<const UObject*> WorldContextObj,
												   const Faerie::ItemData::FValidatedDataView& ViewA,
												   const Faerie::ItemData::FValidatedDataView& ViewB) const
{
	return Execute(ViewA.DataView, ViewB.DataView);
}