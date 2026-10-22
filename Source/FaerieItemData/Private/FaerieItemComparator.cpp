// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemComparator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemComparator)

using namespace Faerie;

bool UFaerieItemComparator::K2_Exec(const FFaerieItemProxy& A, const FFaerieItemProxy& B) const
{
	if (A.HasValidInstance() && B.HasValidInstance())
	{
		return Exec(A, B);
	}
	return false;
}

bool UFaerieItemComparator_BlueprintBase::Exec(const TValid<const FFaerieItemProxy&> ProxyA,
											   const TValid<const FFaerieItemProxy&> ProxyB) const
{
	return Execute(ValidGet(ProxyA), ValidGet(ProxyB));
}