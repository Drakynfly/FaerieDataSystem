// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "BasicItemComparators.h"
#include "FaerieItem.h"
#include "Tokens/FaerieInfoToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BasicItemComparators)

bool UFaerieLexicographicNameComparator::Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const
{
	const UFaerieInfoToken* InfoA = A.ItemObject->GetToken<UFaerieInfoToken>();
	const UFaerieInfoToken* InfoB = B.ItemObject->GetToken<UFaerieInfoToken>();

	if (IsValid(InfoA) && IsValid(InfoB))
	{
		return InfoA->GetItemName().ToString() < InfoB->GetItemName().ToString();
	}

	return false;
}

bool UFaerieDateModifiedComparator::Exec(const FFaerieItemSnapshot& A, const FFaerieItemSnapshot& B) const
{
	return A.ItemObject->GetLastModified() < B.ItemObject->GetLastModified();
}