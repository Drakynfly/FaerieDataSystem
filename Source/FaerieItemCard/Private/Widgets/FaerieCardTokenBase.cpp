// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Widgets/FaerieCardTokenBase.h"

#include "FaerieItem.h"
#include "FaerieItemToken.h"
#include "Widgets/FaerieCardBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FaerieCardTokenBase)

void UFaerieCardTokenBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().AddUObject(this, &ThisClass::OnCardRefreshed);
	}
}

void UFaerieCardTokenBase::NativeDestruct()
{
	if (auto&& OuterCard = GetTypedOuter<UFaerieCardBase>())
	{
		OuterCard->GetOnCardRefreshed().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UFaerieCardTokenBase::OnCardRefreshed()
{
	BP_Refresh();
}

const UFaerieItem* UFaerieCardTokenBase::GetItem() const
{
	if (auto&& Card = GetOwningCard())
	{
		if (auto&& ItemObj = Card->GetItemData().GetItemObject();
			IsValid(ItemObj))
		{
			return ItemObj;
		}
	}
	return nullptr;
}

FFaerieItemProxy UFaerieCardTokenBase::GetProxy() const
{
	if (auto&& Card = GetOwningCard())
	{
		return Card->GetItemData();
	}
	return nullptr;
}

UFaerieCardBase* UFaerieCardTokenBase::GetOwningCard() const
{
	return GetTypedOuter<UFaerieCardBase>();
}

UFaerieItemToken* UFaerieCardTokenBase::GetItemToken(const TSubclassOf<UFaerieItemToken> Class) const
{
	if (auto&& ItemObj = GetItem())
	{
		// @Note: BP doesn't understand const-ness, but since UFaerieItemToken does not have a BP accessible API that can
		// mutate it, it's perfectly safe.
		return const_cast<UFaerieItemToken*>(ItemObj->GetToken(Class));
	}
	return nullptr;
}

bool UFaerieCardTokenBase::GetItemTokenChecked(UFaerieItemToken*& Token, const TSubclassOf<UFaerieItemToken> Class) const
{
	Token = GetItemToken(Class);
	return IsValid(Token);
}