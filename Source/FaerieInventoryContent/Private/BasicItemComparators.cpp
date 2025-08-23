// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemComparators.h"
#include "FaerieItem.h"
#include "Tokens/FaerieInfoToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemComparators)

bool UFaerieLexicographicNameComparator::Exec(const FFaerieItemProxy A, const FFaerieItemProxy B) const
{
	if (!A.IsValid() || !B.IsValid()) return false;

	const UFaerieInfoToken* InfoA = A->GetItemObject()->GetToken<UFaerieInfoToken>();
	const UFaerieInfoToken* InfoB = B->GetItemObject()->GetToken<UFaerieInfoToken>();

	if (IsValid(InfoA) && IsValid(InfoB))
	{
		return InfoA->GetItemName().ToString() < InfoB->GetItemName().ToString();
	}

	return false;
}

bool UFaerieDateModifiedComparator::Exec(const FFaerieItemProxy A, const FFaerieItemProxy B) const
{
	return A->GetItemObject()->GetLastModified() < B->GetItemObject()->GetLastModified();
}