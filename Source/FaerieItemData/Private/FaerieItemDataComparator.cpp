// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataComparator.h"
#include "FaerieItemDataViewWrapper.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemDataComparator)

bool UFaerieItemDataComparator::K2_Exec(const FFaerieItemDataViewWrapper& A,
										const FFaerieItemDataViewWrapper& B) const
{
	if (A.ViewPointer && B.ViewPointer)
	{
		return Exec(A.ViewPointer, B.ViewPointer);
	}
	return false;
}

bool UFaerieItemDataComparator_BlueprintBase::Exec(Faerie::ItemData::FViewPtr ViewA, Faerie::ItemData::FViewPtr ViewB) const
{
	return Execute(FFaerieItemDataViewWrapper(ViewA), FFaerieItemDataViewWrapper(ViewB));
}
