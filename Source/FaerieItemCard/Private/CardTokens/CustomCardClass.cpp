// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "GameplayTagContainer.h"
#include "CardTokens/FaerieItemCardToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieItemCardToken)

TSoftClassPtr<UFaerieCardBase> UFaerieItemCardToken::GetCardClass(const FGameplayTag Tag) const
{
	// @todo tag loop
	if (auto Class = CardClasses.Find(Tag))
	{
		return *Class;
	}
	return nullptr;
}
