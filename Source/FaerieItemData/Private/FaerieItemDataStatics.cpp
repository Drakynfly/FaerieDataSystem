// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "FaerieItemDataStatics.h"
#include "FaerieItem.h"
#include "FaerieItemToken.h"

namespace Faerie
{
	bool ValidateLoadedItem(const UFaerieItem* Item)
	{
		if (!IsValid(Item))
		{
			UE_LOG(LogTemp, Error, TEXT("ValidateLoadedItem: Item pointer is invalid."))
			return false;
		}

		bool HitError = false;

		auto Tokens = Item->GetTokens();
		for (int32 i = 0; i < Tokens.Num(); ++i)
		{
			if (!IsValid(Tokens[i]))
			{
				UE_LOG(LogTemp, Error, TEXT("ValidateLoadedItem: Token[%i] is invalid."), i)
				HitError |= true;
			}
		}

		return !HitError;
	}
}
